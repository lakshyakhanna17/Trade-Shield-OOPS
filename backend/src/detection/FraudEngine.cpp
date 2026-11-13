#include "FraudEngine.hpp"
#include "BaseDetectionRule.hpp"
#include "HighVolumeRule.hpp"
#include "RepeatedCancellationRule.hpp"
#include "HighFrequencyRule.hpp"
#include <iostream>

namespace tradeshield {

static int severityRank(AlertSeverity sev) {
    switch (sev) {
        case AlertSeverity::LOW: return 1;
        case AlertSeverity::MEDIUM: return 2;
        case AlertSeverity::HIGH: return 3;
    }
    return 0;
}

FraudEngine::FraudEngine(AlertRepository& alert_repo,
                         TraderRepository& trader_repo,
                         OrderRepository& order_repo,
                         TradeRepository& trade_repo)
    : alert_repo_(alert_repo),
      trader_repo_(trader_repo),
      order_repo_(order_repo),
      trade_repo_(trade_repo) {
    registerDefaultRules();
}

void FraudEngine::registerDefaultRules() {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    rules_.clear();
    rules_.push_back(std::make_unique<HighVolumeRule>());
    rules_.push_back(std::make_unique<RepeatedCancellationRule>());
    rules_.push_back(std::make_unique<HighFrequencyRule>());
}

void FraudEngine::addRule(std::unique_ptr<IDetectionRule> rule) {
    if (!rule) return;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    rules_.push_back(std::move(rule));
}

void FraudEngine::clearRules() {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    rules_.clear();
}

size_t FraudEngine::getRuleCount() const noexcept {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return rules_.size();
}

std::vector<Alert> FraudEngine::processEvent(const TradingEvent& event) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    std::vector<Alert> triggered_alerts;

    try {
        auto trader_opt = trader_repo_.findById(event.trader_id);
        if (!trader_opt.has_value()) {
            return triggered_alerts;
        }

        const auto& trader = *trader_opt;

        DetectionContext ctx;
        ctx.trader_id = event.trader_id;
        ctx.trader_name = trader->getName();
        ctx.trader_type = trader->getTraderType();
        ctx.cash_balance_paise = trader->getCashBalancePaise();
        ctx.current_event = event;
        ctx.recent_orders = order_repo_.findByTraderId(event.trader_id, 100);
        ctx.recent_trades = trade_repo_.findByTraderId(event.trader_id, 100);
        ctx.window_seconds = 60;

        for (const auto& rule : rules_) {
            if (!rule) continue;

            DetectionResult res = rule->evaluate(ctx);
            if (res.triggered) {
                // Prevent duplicate alerts for the exact same order and rule
                if (event.order_id > 0 &&
                    alert_repo_.existsByOrderAndRule(event.order_id, res.rule_name)) {
                    continue;
                }

                std::optional<int64_t> opt_order_id = event.order_id > 0
                    ? std::make_optional(event.order_id)
                    : std::nullopt;

                // Check for active open incident for this trader and rule
                auto active_incident = alert_repo_.findActiveIncident(event.trader_id, res.rule_name);
                bool should_aggregate = false;

                if (active_incident.has_value()) {
                    int64_t event_time = BaseDetectionRule::parseIsoTimestamp(event.timestamp);
                    int64_t last_time = BaseDetectionRule::parseIsoTimestamp(active_incident->getLastSeenAt());
                    int64_t delta = (event_time >= last_time) ? (event_time - last_time) : (last_time - event_time);
                    if (event_time == 0 || last_time == 0 || delta <= episode_window_seconds_) {
                        should_aggregate = true;
                    }
                }

                if (should_aggregate) {
                    // Enrich existing ongoing incident (without duplicate row creation)
                    auto inc = *active_incident;
                    int new_count = inc.getOccurrenceCount() + 1;
                    std::string new_last_seen = event.timestamp.empty() ? inc.getLastSeenAt() : event.timestamp;
                    std::optional<int64_t> latest_order = opt_order_id.has_value() ? opt_order_id : inc.getLatestOrderId();

                    // Severity escalation
                    AlertSeverity final_severity = inc.getSeverity();
                    int final_risk_points = std::max(inc.getRiskPoints(), res.risk_points);
                    if (severityRank(res.severity) > severityRank(inc.getSeverity())) {
                        final_severity = res.severity;
                    }

                    // Preserve evidence and append episode occurrence summary
                    std::string enriched_desc = res.description;
                    if (new_count > 1) {
                        enriched_desc += " [Episode aggregated: " + std::to_string(new_count) + " occurrences, latest order #" +
                                         (latest_order.has_value() ? std::to_string(*latest_order) : "N/A") + "]";
                    }

                    alert_repo_.updateIncident(inc.getId(), new_last_seen, new_count, latest_order,
                                               final_severity, final_risk_points, enriched_desc);

                    Alert updated_alert(
                        inc.getId(),
                        inc.getTraderId(),
                        inc.getOrderId(),
                        inc.getRuleName(),
                        final_severity,
                        final_risk_points,
                        enriched_desc,
                        inc.getCreatedAt(),
                        inc.getReviewStatus(),
                        inc.getFirstSeenAt(),
                        new_last_seen,
                        new_count,
                        latest_order
                    );
                    triggered_alerts.push_back(std::move(updated_alert));
                } else {
                    // Start brand new incident episode
                    Alert alert(
                        0,
                        event.trader_id,
                        opt_order_id,
                        res.rule_name,
                        res.severity,
                        res.risk_points,
                        res.description,
                        event.timestamp,
                        AlertReviewStatus::OPEN,
                        event.timestamp,
                        event.timestamp,
                        1,
                        opt_order_id
                    );

                    int64_t alert_id = alert_repo_.insert(alert);

                    Alert persisted_alert(
                        alert_id,
                        event.trader_id,
                        opt_order_id,
                        res.rule_name,
                        res.severity,
                        res.risk_points,
                        res.description,
                        event.timestamp,
                        AlertReviewStatus::OPEN,
                        event.timestamp,
                        event.timestamp,
                        1,
                        opt_order_id
                    );
                    triggered_alerts.push_back(std::move(persisted_alert));
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[FraudEngine] Warning: Exception in processEvent: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[FraudEngine] Warning: Unknown exception in processEvent" << std::endl;
    }

    return triggered_alerts;
}

std::optional<TraderRiskProfile> FraudEngine::getTraderRisk(int64_t trader_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto trader_opt = trader_repo_.findById(trader_id);
    if (!trader_opt.has_value()) {
        return std::nullopt;
    }

    auto alerts = alert_repo_.findByTraderId(trader_id, 500);
    return RiskScorer::buildProfile(trader_id, (*trader_opt)->getName(), alerts);
}

std::vector<TraderRiskProfile> FraudEngine::getAllTradersRisk() {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    std::vector<TraderRiskProfile> profiles;

    auto traders = trader_repo_.findAll();
    for (const auto& trader : traders) {
        if (!trader) continue;
        auto alerts = alert_repo_.findByTraderId(trader->getId(), 500);
        profiles.push_back(RiskScorer::buildProfile(trader->getId(), trader->getName(), alerts));
    }
    return profiles;
}

} // namespace tradeshield
