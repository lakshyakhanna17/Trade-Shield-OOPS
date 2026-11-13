#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Portfolio.hpp"
#include "models/PortfolioSnapshot.hpp"
#include <optional>
#include <vector>

namespace tradeshield {

/**
 * @brief Repository for persisting and querying Portfolio valuation summaries and historical snapshots.
 */
class PortfolioRepository {
public:
    explicit PortfolioRepository(DatabaseManager& db) : db_(db) {}

    std::optional<Portfolio> findByTraderId(int64_t trader_id);
    int64_t saveOrUpdate(const Portfolio& portfolio);
    bool updateValue(int64_t trader_id, int64_t total_value_paise, const std::string& updated_at);

    // Snapshot operations for real historical performance charting
    int64_t saveSnapshot(int64_t trader_id, int64_t total_value_paise,
                         int64_t cash_balance_paise, int64_t holdings_value_paise,
                         const std::string& snapshot_time);
    std::vector<PortfolioSnapshot> getSnapshotsByTraderId(int64_t trader_id, int limit = 100);
    int64_t getSnapshotCount(int64_t trader_id);

private:
    DatabaseManager& db_;
    Portfolio mapRowToPortfolio(Statement& stmt);
    PortfolioSnapshot mapRowToSnapshot(Statement& stmt);
};

} // namespace tradeshield
