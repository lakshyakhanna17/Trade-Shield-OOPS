#include "AlertRepository.hpp"

namespace tradeshield {

Alert AlertRepository::mapRowToAlert(Statement& stmt) {
    int64_t alert_id = stmt.getInt64(0);
    int64_t trader_id = stmt.getInt64(1);
    std::optional<int64_t> order_id = stmt.getOptionalInt64(2);
    std::string rule_name = stmt.getText(3);
    AlertSeverity severity = stringToAlertSeverity(stmt.getText(4));
    int risk_points = stmt.getInt(5);
    std::string description = stmt.getText(6);
    std::string created_at = stmt.getText(7);
    AlertReviewStatus review_status = stringToAlertReviewStatus(stmt.getText(8));
    std::string first_seen_at = stmt.getText(9);
    std::string last_seen_at = stmt.getText(10);
    int occurrence_count = stmt.getInt(11);
    std::optional<int64_t> latest_order_id = stmt.getOptionalInt64(12);

    if (first_seen_at.empty()) first_seen_at = created_at;
    if (last_seen_at.empty()) last_seen_at = created_at;
    if (occurrence_count <= 0) occurrence_count = 1;
    if (!latest_order_id.has_value()) latest_order_id = order_id;

    return Alert(alert_id, trader_id, order_id, std::move(rule_name), severity, risk_points,
                 std::move(description), std::move(created_at), review_status,
                 std::move(first_seen_at), std::move(last_seen_at), occurrence_count, latest_order_id);
}

std::optional<Alert> AlertRepository::findById(int64_t alert_id) {
    auto stmt = db_.prepare(
        "SELECT alert_id, trader_id, order_id, rule_name, severity, risk_points, description, created_at, review_status, "
        "first_seen_at, last_seen_at, occurrence_count, latest_order_id "
        "FROM alerts WHERE alert_id = ?;"
    );
    stmt.bindInt64(1, alert_id);
    if (stmt.step()) {
        return mapRowToAlert(stmt);
    }
    return std::nullopt;
}

std::vector<Alert> AlertRepository::findByTraderId(int64_t trader_id, int limit) {
    std::vector<Alert> alerts;
    auto stmt = db_.prepare(
        "SELECT alert_id, trader_id, order_id, rule_name, severity, risk_points, description, created_at, review_status, "
        "first_seen_at, last_seen_at, occurrence_count, latest_order_id "
        "FROM alerts WHERE trader_id = ? ORDER BY alert_id DESC LIMIT ?;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt(2, limit);
    while (stmt.step()) {
        alerts.push_back(mapRowToAlert(stmt));
    }
    return alerts;
}

std::vector<Alert> AlertRepository::findAll(int limit) {
    return findByFilters(std::nullopt, std::nullopt, std::nullopt, limit, 0);
}

std::vector<Alert> AlertRepository::findByFilters(std::optional<int64_t> trader_id,
                                                 std::optional<AlertSeverity> severity,
                                                 std::optional<AlertReviewStatus> review_status,
                                                 int limit,
                                                 int offset) {
    std::vector<Alert> alerts;
    std::string sql = "SELECT alert_id, trader_id, order_id, rule_name, severity, risk_points, description, created_at, review_status, "
                      "first_seen_at, last_seen_at, occurrence_count, latest_order_id "
                      "FROM alerts WHERE 1=1";
    if (trader_id.has_value()) {
        sql += " AND trader_id = ?";
    }
    if (severity.has_value()) {
        sql += " AND severity = ?";
    }
    if (review_status.has_value()) {
        sql += " AND review_status = ?";
    }
    sql += " ORDER BY alert_id DESC LIMIT ? OFFSET ?;";

    auto stmt = db_.prepare(sql);
    int bind_idx = 1;
    if (trader_id.has_value()) {
        stmt.bindInt64(bind_idx++, *trader_id);
    }
    if (severity.has_value()) {
        stmt.bindText(bind_idx++, alertSeverityToString(*severity));
    }
    if (review_status.has_value()) {
        stmt.bindText(bind_idx++, alertReviewStatusToString(*review_status));
    }
    stmt.bindInt(bind_idx++, limit);
    stmt.bindInt(bind_idx++, offset);

    while (stmt.step()) {
        alerts.push_back(mapRowToAlert(stmt));
    }
    return alerts;
}

int64_t AlertRepository::countByFilters(std::optional<int64_t> trader_id,
                                        std::optional<AlertSeverity> severity,
                                        std::optional<AlertReviewStatus> review_status) {
    std::string sql = "SELECT COUNT(*) FROM alerts WHERE 1=1";
    if (trader_id.has_value()) {
        sql += " AND trader_id = ?";
    }
    if (severity.has_value()) {
        sql += " AND severity = ?";
    }
    if (review_status.has_value()) {
        sql += " AND review_status = ?";
    }

    auto stmt = db_.prepare(sql);
    int bind_idx = 1;
    if (trader_id.has_value()) {
        stmt.bindInt64(bind_idx++, *trader_id);
    }
    if (severity.has_value()) {
        stmt.bindText(bind_idx++, alertSeverityToString(*severity));
    }
    if (review_status.has_value()) {
        stmt.bindText(bind_idx++, alertReviewStatusToString(*review_status));
    }

    if (stmt.step()) {
        return stmt.getInt64(0);
    }
    return 0;
}

std::optional<Alert> AlertRepository::findActiveIncident(int64_t trader_id, const std::string& rule_name) {
    auto stmt = db_.prepare(
        "SELECT alert_id, trader_id, order_id, rule_name, severity, risk_points, description, created_at, review_status, "
        "first_seen_at, last_seen_at, occurrence_count, latest_order_id "
        "FROM alerts WHERE trader_id = ? AND rule_name = ? AND review_status = 'OPEN' "
        "ORDER BY alert_id DESC LIMIT 1;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindText(2, rule_name);
    if (stmt.step()) {
        return mapRowToAlert(stmt);
    }
    return std::nullopt;
}

bool AlertRepository::updateIncident(int64_t alert_id, const std::string& last_seen_at, int occurrence_count,
                                     std::optional<int64_t> latest_order_id, AlertSeverity severity,
                                     int risk_points, const std::string& description) {
    auto stmt = db_.prepare(
        "UPDATE alerts SET last_seen_at = ?, occurrence_count = ?, latest_order_id = ?, severity = ?, risk_points = ?, description = ? "
        "WHERE alert_id = ?;"
    );
    stmt.bindText(1, last_seen_at);
    stmt.bindInt(2, occurrence_count);
    stmt.bindOptionalInt64(3, latest_order_id);
    stmt.bindText(4, alertSeverityToString(severity));
    stmt.bindInt(5, risk_points);
    stmt.bindText(6, description);
    stmt.bindInt64(7, alert_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

AlertStats AlertRepository::getStats() {
    AlertStats stats;
    auto stmt = db_.prepare(
        "SELECT "
        "  COUNT(*), "
        "  COALESCE(SUM(CASE WHEN review_status = 'OPEN' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(SUM(CASE WHEN review_status = 'REVIEWED' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(SUM(CASE WHEN review_status = 'CLEARED' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(SUM(CASE WHEN severity = 'HIGH' AND review_status = 'OPEN' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(SUM(CASE WHEN severity = 'MEDIUM' AND review_status = 'OPEN' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(SUM(CASE WHEN severity = 'LOW' AND review_status = 'OPEN' THEN 1 ELSE 0 END), 0) "
        "FROM alerts;"
    );
    if (stmt.step()) {
        stats.total_alerts = stmt.getInt64(0);
        stats.open_alerts = stmt.getInt64(1);
        stats.reviewed_alerts = stmt.getInt64(2);
        stats.cleared_alerts = stmt.getInt64(3);
        stats.high_severity_alerts = stmt.getInt64(4);
        stats.medium_severity_alerts = stmt.getInt64(5);
        stats.low_severity_alerts = stmt.getInt64(6);
        stats.active_incidents = stats.open_alerts;
    }
    return stats;
}

bool AlertRepository::existsByOrderAndRule(int64_t order_id, const std::string& rule_name) {
    auto stmt = db_.prepare(
        "SELECT 1 FROM alerts WHERE order_id = ? AND rule_name = ? LIMIT 1;"
    );
    stmt.bindInt64(1, order_id);
    stmt.bindText(2, rule_name);
    return stmt.step();
}

int64_t AlertRepository::insert(const Alert& alert) {
    auto stmt = db_.prepare(
        "INSERT INTO alerts (trader_id, order_id, rule_name, severity, risk_points, description, created_at, review_status, "
        "first_seen_at, last_seen_at, occurrence_count, latest_order_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
    );
    stmt.bindInt64(1, alert.getTraderId());
    stmt.bindOptionalInt64(2, alert.getOrderId());
    stmt.bindText(3, alert.getRuleName());
    stmt.bindText(4, alertSeverityToString(alert.getSeverity()));
    stmt.bindInt(5, alert.getRiskPoints());
    stmt.bindText(6, alert.getDescription());
    stmt.bindText(7, alert.getCreatedAt());
    stmt.bindText(8, alertReviewStatusToString(alert.getReviewStatus()));
    stmt.bindText(9, alert.getFirstSeenAt().empty() ? alert.getCreatedAt() : alert.getFirstSeenAt());
    stmt.bindText(10, alert.getLastSeenAt().empty() ? alert.getCreatedAt() : alert.getLastSeenAt());
    stmt.bindInt(11, std::max(1, alert.getOccurrenceCount()));
    stmt.bindOptionalInt64(12, alert.getLatestOrderId().has_value() ? alert.getLatestOrderId() : alert.getOrderId());

    stmt.execute();
    return db_.getLastInsertRowId();
}

bool AlertRepository::updateReviewStatus(int64_t alert_id, AlertReviewStatus new_status) {
    auto stmt = db_.prepare(
        "UPDATE alerts SET review_status = ? WHERE alert_id = ?;"
    );
    stmt.bindText(1, alertReviewStatusToString(new_status));
    stmt.bindInt64(2, alert_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

} // namespace tradeshield
