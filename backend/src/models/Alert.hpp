#pragma once

#include "Types.hpp"
#include <string>
#include <optional>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Represents a fraud or anomaly detection alert flagged by the Shield engine.
 */
class Alert {
public:
    Alert(int64_t alert_id, int64_t trader_id, std::optional<int64_t> order_id,
          std::string rule_name, AlertSeverity severity, int risk_points,
          std::string description, std::string created_at,
          AlertReviewStatus review_status = AlertReviewStatus::OPEN,
          std::string first_seen_at = "", std::string last_seen_at = "",
          int occurrence_count = 1,
          std::optional<int64_t> latest_order_id = std::nullopt);

    int64_t getId() const noexcept { return alert_id_; }
    int64_t getTraderId() const noexcept { return trader_id_; }
    const std::optional<int64_t>& getOrderId() const noexcept { return order_id_; }
    const std::string& getRuleName() const noexcept { return rule_name_; }
    AlertSeverity getSeverity() const noexcept { return severity_; }
    int getRiskPoints() const noexcept { return risk_points_; }
    const std::string& getDescription() const noexcept { return description_; }
    const std::string& getCreatedAt() const noexcept { return created_at_; }
    AlertReviewStatus getReviewStatus() const noexcept { return review_status_; }
    const std::string& getFirstSeenAt() const noexcept { return first_seen_at_; }
    const std::string& getLastSeenAt() const noexcept { return last_seen_at_; }
    int getOccurrenceCount() const noexcept { return occurrence_count_; }
    const std::optional<int64_t>& getLatestOrderId() const noexcept { return latest_order_id_; }

    void setReviewStatus(AlertReviewStatus new_status) noexcept { review_status_ = new_status; }
    void setSeverity(AlertSeverity new_severity) noexcept { severity_ = new_severity; }
    void setRiskPoints(int points) noexcept { risk_points_ = points; }
    void setDescription(std::string desc) noexcept { description_ = std::move(desc); }
    void setLastSeenAt(std::string timestamp) noexcept { last_seen_at_ = std::move(timestamp); }
    void setLatestOrderId(std::optional<int64_t> order_id) noexcept { latest_order_id_ = order_id; }
    void incrementOccurrenceCount(int count = 1) noexcept { occurrence_count_ += count; }

private:
    int64_t alert_id_{0};
    int64_t trader_id_{0};
    std::optional<int64_t> order_id_;
    std::string rule_name_;
    AlertSeverity severity_{AlertSeverity::LOW};
    int risk_points_{0};
    std::string description_;
    std::string created_at_;
    AlertReviewStatus review_status_{AlertReviewStatus::OPEN};
    std::string first_seen_at_;
    std::string last_seen_at_;
    int occurrence_count_{1};
    std::optional<int64_t> latest_order_id_;
};

} // namespace tradeshield
