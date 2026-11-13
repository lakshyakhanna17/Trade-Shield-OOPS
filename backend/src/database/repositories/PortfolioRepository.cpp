#include "PortfolioRepository.hpp"

namespace tradeshield {

Portfolio PortfolioRepository::mapRowToPortfolio(Statement& stmt) {
    int64_t id = stmt.getInt64(0);
    int64_t trader_id = stmt.getInt64(1);
    int64_t total_val = stmt.getInt64(2);
    std::string updated_at = stmt.getText(3);

    return Portfolio(id, trader_id, total_val, std::move(updated_at));
}

std::optional<Portfolio> PortfolioRepository::findByTraderId(int64_t trader_id) {
    auto stmt = db_.prepare(
        "SELECT portfolio_id, trader_id, total_value_paise, updated_at "
        "FROM portfolios WHERE trader_id = ?;"
    );
    stmt.bindInt64(1, trader_id);
    if (stmt.step()) {
        return mapRowToPortfolio(stmt);
    }
    return std::nullopt;
}

int64_t PortfolioRepository::saveOrUpdate(const Portfolio& portfolio) {
    auto stmt = db_.prepare(
        "INSERT INTO portfolios (trader_id, total_value_paise, updated_at) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(trader_id) DO UPDATE SET "
        "total_value_paise = excluded.total_value_paise, "
        "updated_at = excluded.updated_at;"
    );
    stmt.bindInt64(1, portfolio.getTraderId());
    stmt.bindInt64(2, portfolio.getTotalValuePaise());
    stmt.bindText(3, portfolio.getUpdatedAt());

    stmt.execute();
    return db_.getLastInsertRowId();
}

bool PortfolioRepository::updateValue(int64_t trader_id, int64_t total_value_paise, const std::string& updated_at) {
    if (total_value_paise < 0) {
        throw ValidationException("Portfolio total value cannot be negative");
    }
    auto stmt = db_.prepare(
        "UPDATE portfolios SET total_value_paise = ?, updated_at = ? WHERE trader_id = ?;"
    );
    stmt.bindInt64(1, total_value_paise);
    stmt.bindText(2, updated_at);
    stmt.bindInt64(3, trader_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

PortfolioSnapshot PortfolioRepository::mapRowToSnapshot(Statement& stmt) {
    int64_t id = stmt.getInt64(0);
    int64_t trader_id = stmt.getInt64(1);
    int64_t total_val = stmt.getInt64(2);
    int64_t cash = stmt.getInt64(3);
    int64_t holdings = stmt.getInt64(4);
    std::string snap_time = stmt.getText(5);

    return PortfolioSnapshot(id, trader_id, total_val, cash, holdings, std::move(snap_time));
}

int64_t PortfolioRepository::saveSnapshot(int64_t trader_id, int64_t total_value_paise,
                                         int64_t cash_balance_paise, int64_t holdings_value_paise,
                                         const std::string& snapshot_time) {
    if (total_value_paise < 0 || cash_balance_paise < 0 || holdings_value_paise < 0) {
        throw ValidationException("Snapshot values cannot be negative");
    }
    auto stmt = db_.prepare(
        "INSERT INTO portfolio_snapshots (trader_id, total_value_paise, cash_balance_paise, holdings_value_paise, snapshot_time) "
        "VALUES (?, ?, ?, ?, ?);"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt64(2, total_value_paise);
    stmt.bindInt64(3, cash_balance_paise);
    stmt.bindInt64(4, holdings_value_paise);
    stmt.bindText(5, snapshot_time);
    stmt.execute();
    return db_.getLastInsertRowId();
}

std::vector<PortfolioSnapshot> PortfolioRepository::getSnapshotsByTraderId(int64_t trader_id, int limit) {
    auto stmt = db_.prepare(
        "SELECT snapshot_id, trader_id, total_value_paise, cash_balance_paise, holdings_value_paise, snapshot_time "
        "FROM portfolio_snapshots WHERE trader_id = ? ORDER BY snapshot_time ASC, snapshot_id ASC LIMIT ?;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt64(2, limit);

    std::vector<PortfolioSnapshot> results;
    while (stmt.step()) {
        results.push_back(mapRowToSnapshot(stmt));
    }
    return results;
}

int64_t PortfolioRepository::getSnapshotCount(int64_t trader_id) {
    auto stmt = db_.prepare(
        "SELECT COUNT(*) FROM portfolio_snapshots WHERE trader_id = ?;"
    );
    stmt.bindInt64(1, trader_id);
    if (stmt.step()) {
        return stmt.getInt64(0);
    }
    return 0;
}

} // namespace tradeshield
