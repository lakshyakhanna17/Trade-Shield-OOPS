#include "HighVolumeRule.hpp"

namespace tradeshield {

HighVolumeRule::HighVolumeRule(int64_t quantity_threshold, int64_t value_threshold_paise, double spike_multiplier)
    : BaseDetectionRule("RULE_HIGH_VOLUME", "HIGH_VOLUME",
                         "Detects unusually large trading volume or repeated abnormal volume spikes"),
      quantity_threshold_(quantity_threshold),
      value_threshold_paise_(value_threshold_paise),
      spike_multiplier_(spike_multiplier) {}

DetectionResult HighVolumeRule::evaluate(const DetectionContext& ctx) {
    if (ctx.current_event.type != TradingEventType::ORDER_SUBMITTED &&
        ctx.current_event.type != TradingEventType::ORDER_EXECUTED) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    int64_t current_qty = ctx.current_event.quantity;
    int64_t current_price = ctx.current_event.price_paise;
    int64_t current_val_paise = current_qty * current_price;

    bool exceeds_qty = current_qty >= quantity_threshold_;
    bool exceeds_val = current_val_paise >= value_threshold_paise_;

    if (!exceeds_qty && !exceeds_val) {
        return DetectionResult::notTriggered(rule_id_, rule_name_);
    }

    // Inspect trader's historical orders to determine if this is isolated vs repeated
    int large_orders_count = 1; // Current order is large
    int64_t total_past_qty = 0;
    int past_orders_count = 0;

    int64_t event_epoch = parseIsoTimestamp(ctx.current_event.timestamp);

    for (const auto& ord : ctx.recent_orders) {
        // Exclude the current order if already in recent_orders
        if (ord.getId() == ctx.current_event.order_id) {
            continue;
        }

        total_past_qty += ord.getQuantity();
        past_orders_count++;

        int64_t ord_epoch = parseIsoTimestamp(ord.getCreatedAt());
        if (event_epoch - ord_epoch <= ctx.window_seconds) {
            int64_t ord_val = ord.getTotalValuePaise();
            if (ord.getQuantity() >= quantity_threshold_ || ord_val >= value_threshold_paise_) {
                large_orders_count++;
            }
        }
    }

    DetectionResult res;
    res.rule_id = rule_id_;
    res.rule_name = rule_name_;
    res.triggered = true;
    res.evidence["current_quantity"] = std::to_string(current_qty);
    res.evidence["current_value_paise"] = std::to_string(current_val_paise);
    res.evidence["quantity_threshold"] = std::to_string(quantity_threshold_);
    res.evidence["large_orders_in_window"] = std::to_string(large_orders_count);

    double average_past_qty = past_orders_count > 0
        ? static_cast<double>(total_past_qty) / past_orders_count
        : static_cast<double>(current_qty);

    bool is_spike_over_average = (past_orders_count >= 2) &&
        (static_cast<double>(current_qty) >= average_past_qty * spike_multiplier_);

    if (large_orders_count >= 3) {
        // Repeated high volume bursts
        res.severity = AlertSeverity::HIGH;
        res.risk_points = 65;
        res.description = "Persistent abnormal volume detected: " + std::to_string(large_orders_count) +
                          " large orders submitted in recent window (current qty: " + std::to_string(current_qty) + ").";
    } else if (large_orders_count == 2 || is_spike_over_average) {
        // Multiple large orders or significant spike relative to history
        res.severity = AlertSeverity::MEDIUM;
        res.risk_points = 45;
        res.description = "Unusually high order volume detected: order qty " + std::to_string(current_qty) +
                          " is " + (is_spike_over_average ? "over 3x historical average" : "part of repeated large orders") + ".";
    } else {
        // Isolated single large order: strictly LOW risk per requirements
        res.severity = AlertSeverity::LOW;
        res.risk_points = 20;
        res.description = "Single high volume order placed (qty: " + std::to_string(current_qty) +
                          ", value: " + std::to_string(current_val_paise / 100) + " INR). Monitored as isolated large order.";
    }

    return res;
}

} // namespace tradeshield
