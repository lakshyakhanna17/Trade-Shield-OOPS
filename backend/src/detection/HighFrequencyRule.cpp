#include "HighFrequencyRule.hpp"

namespace tradeshield {

HighFrequencyRule::HighFrequencyRule(int window_seconds, int burst_order_threshold)
    : BaseDetectionRule("RULE_HIGH_FREQUENCY", "HIGH_FREQUENCY",
                         "Detects unusually rapid bursts of order submissions exceeding normal execution frequency"),
      window_seconds_(window_seconds),
      burst_order_threshold_(burst_order_threshold) {}

DetectionResult HighFrequencyRule::evaluate(const DetectionContext& ctx) {
    if (ctx.current_event.type != TradingEventType::ORDER_SUBMITTED) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    int64_t event_epoch = parseIsoTimestamp(ctx.current_event.timestamp);

    int orders_in_window = 1; // Count the current order submission

    for (const auto& ord : ctx.recent_orders) {
        if (ord.getId() == ctx.current_event.order_id) {
            continue;
        }

        int64_t ord_epoch = parseIsoTimestamp(ord.getCreatedAt());
        int64_t delta_seconds = event_epoch - ord_epoch;

        if (delta_seconds >= -5 && delta_seconds <= window_seconds_) {
            orders_in_window++;
        }
    }

    if (orders_in_window < burst_order_threshold_) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    DetectionResult res;
    res.rule_id = rule_id_;
    res.rule_name = rule_name_;
    res.triggered = true;
    res.evidence["orders_in_window"] = std::to_string(orders_in_window);
    res.evidence["window_seconds"] = std::to_string(window_seconds_);
    res.evidence["burst_order_threshold"] = std::to_string(burst_order_threshold_);

    if (orders_in_window >= burst_order_threshold_ * 2) {
        res.severity = AlertSeverity::HIGH;
        res.risk_points = 75;
        res.description = "Severe high-frequency burst detected: " + std::to_string(orders_in_window) +
                          " orders submitted within " + std::to_string(window_seconds_) + " seconds.";
    } else {
        res.severity = AlertSeverity::MEDIUM;
        res.risk_points = 50;
        res.description = "Elevated order frequency detected: " + std::to_string(orders_in_window) +
                          " orders submitted within " + std::to_string(window_seconds_) + " seconds.";
    }

    return res;
}

} // namespace tradeshield
