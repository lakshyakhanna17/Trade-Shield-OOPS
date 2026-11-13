#include "RepeatedCancellationRule.hpp"
#include <iomanip>
#include <sstream>

namespace tradeshield {

RepeatedCancellationRule::RepeatedCancellationRule(int window_seconds,
                                                   int min_cancellations_threshold,
                                                   double cancellation_ratio_threshold)
    : BaseDetectionRule("RULE_REPEATED_CANCELLATION", "REPEATED_CANCELLATION",
                         "Detects suspicious patterns of repeated order cancellations and high cancellation ratios"),
      window_seconds_(window_seconds),
      min_cancellations_threshold_(min_cancellations_threshold),
      cancellation_ratio_threshold_(cancellation_ratio_threshold) {}

DetectionResult RepeatedCancellationRule::evaluate(const DetectionContext& ctx) {
    if (ctx.current_event.type != TradingEventType::ORDER_CANCELLED) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    int64_t event_epoch = parseIsoTimestamp(ctx.current_event.timestamp);

    int cancellations_in_window = 1; // Count the current cancellation event
    int total_orders_in_window = 1;

    for (const auto& ord : ctx.recent_orders) {
        if (ord.getId() == ctx.current_event.order_id) {
            continue;
        }

        int64_t ord_epoch = parseIsoTimestamp(ord.getUpdatedAt().empty() ? ord.getCreatedAt() : ord.getUpdatedAt());
        int64_t delta_seconds = event_epoch - ord_epoch;

        // Check if within the configurable window (accounting for minor negative skew)
        if (delta_seconds >= -5 && delta_seconds <= window_seconds_) {
            total_orders_in_window++;
            if (ord.getStatus() == OrderStatus::CANCELLED) {
                cancellations_in_window++;
            }
        }
    }

    // Normal occasional cancellations do not trigger the rule
    if (cancellations_in_window < min_cancellations_threshold_) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    double cancel_ratio = total_orders_in_window > 0
        ? static_cast<double>(cancellations_in_window) / total_orders_in_window
        : 1.0;

    if (cancel_ratio < cancellation_ratio_threshold_) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    DetectionResult res;
    res.rule_id = rule_id_;
    res.rule_name = rule_name_;
    res.triggered = true;
    res.evidence["cancellations_in_window"] = std::to_string(cancellations_in_window);
    res.evidence["total_orders_in_window"] = std::to_string(total_orders_in_window);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << (cancel_ratio * 100.0) << "%";
    res.evidence["cancellation_ratio"] = ss.str();
    res.evidence["window_seconds"] = std::to_string(window_seconds_);

    if (cancellations_in_window >= 5 || (cancellations_in_window >= 4 && cancel_ratio >= 0.8)) {
        res.severity = AlertSeverity::HIGH;
        res.risk_points = 70;
        res.description = "Rapid cancellation attack pattern: " + std::to_string(cancellations_in_window) +
                          " cancellations in " + std::to_string(window_seconds_) + "s (" + ss.str() + " cancel ratio).";
    } else {
        res.severity = AlertSeverity::MEDIUM;
        res.risk_points = 45;
        res.description = "Suspicious cancellation frequency: " + std::to_string(cancellations_in_window) +
                          " orders cancelled within " + std::to_string(window_seconds_) + "s (" + ss.str() + " cancel ratio).";
    }

    return res;
}

} // namespace tradeshield
