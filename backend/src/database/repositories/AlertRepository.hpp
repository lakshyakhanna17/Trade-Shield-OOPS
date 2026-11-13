#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Alert.hpp"
#include <vector>
#include <optional>

namespace tradeshield {

/**
 * @brief Aggregate statistics across all Shield alerts in SQLite.
 */
struct AlertStats {
    int64_t total_alerts{0};
    int64_t open_alerts{0};
    int64_t reviewed_alerts{0};
    int64_t cleared_alerts{0};
    int64_t high_severity_alerts{0};
    int64_t medium_severity_alerts{0};
    int64_t low_severity_alerts{0};
    int64_t active_incidents{0};
};

/**
 * @brief Repository for persisting and querying Shield fraud and anomaly alerts.
 */
class AlertRepository {
public:
    explicit AlertRepository(DatabaseManager& db) : db_(db) {}

    std::optional<Alert> findById(int64_t alert_id);
    std::vector<Alert> findByTraderId(int64_t trader_id, int limit = 100);
    std::vector<Alert> findAll(int limit = 100);
    std::vector<Alert> findByFilters(std::optional<int64_t> trader_id,
                                     std::optional<AlertSeverity> severity,
                                     std::optional<AlertReviewStatus> review_status,
                                     int limit = 100,
                                     int offset = 0);
    int64_t countByFilters(std::optional<int64_t> trader_id,
                           std::optional<AlertSeverity> severity,
                           std::optional<AlertReviewStatus> review_status);

    std::optional<Alert> findActiveIncident(int64_t trader_id, const std::string& rule_name);
    bool updateIncident(int64_t alert_id, const std::string& last_seen_at, int occurrence_count,
                        std::optional<int64_t> latest_order_id, AlertSeverity severity,
                        int risk_points, const std::string& description);

    AlertStats getStats();

    bool existsByOrderAndRule(int64_t order_id, const std::string& rule_name);
    int64_t insert(const Alert& alert);
    bool updateReviewStatus(int64_t alert_id, AlertReviewStatus new_status);

private:
    DatabaseManager& db_;
    Alert mapRowToAlert(Statement& stmt);
};

} // namespace tradeshield
