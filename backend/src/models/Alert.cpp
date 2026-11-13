#include "Alert.hpp"

namespace tradeshield {

Alert::Alert(int64_t alert_id, int64_t trader_id, std::optional<int64_t> order_id,
             std::string rule_name, AlertSeverity severity, int risk_points,
             std::string description, std::string created_at,
             AlertReviewStatus review_status,
             std::string first_seen_at, std::string last_seen_at,
             int occurrence_count,
             std::optional<int64_t> latest_order_id)
    : alert_id_(alert_id), trader_id_(trader_id), order_id_(order_id),
      rule_name_(std::move(rule_name)), severity_(severity), risk_points_(risk_points),
      description_(std::move(description)), created_at_(std::move(created_at)),
      review_status_(review_status),
      first_seen_at_(first_seen_at.empty() ? created_at_ : std::move(first_seen_at)),
      last_seen_at_(last_seen_at.empty() ? created_at_ : std::move(last_seen_at)),
      occurrence_count_(std::max(1, occurrence_count)),
      latest_order_id_(latest_order_id.has_value() ? latest_order_id : order_id_) {
    if (trader_id_ <= 0) {
        throw ValidationException("Invalid trader ID in alert");
    }
    if (order_id_.has_value() && order_id_.value() <= 0) {
        throw ValidationException("Invalid order ID in alert");
    }
    if (rule_name_.empty()) {
        throw ValidationException("Alert rule name cannot be empty");
    }
    if (risk_points_ < 0 || risk_points_ > 100) {
        throw ValidationException("Alert risk points must be between 0 and 100 inclusive (got " +
                                  std::to_string(risk_points_) + ")");
    }
    if (description_.empty()) {
        throw ValidationException("Alert description cannot be empty");
    }
}

} // namespace tradeshield
