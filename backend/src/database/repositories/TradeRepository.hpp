#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Trade.hpp"
#include <vector>
#include <optional>

namespace tradeshield {

/**
 * @brief Repository for persisting and retrieving executed Trades.
 */
class TradeRepository {
public:
    explicit TradeRepository(DatabaseManager& db) : db_(db) {}

    std::optional<Trade> findById(int64_t trade_id);
    std::vector<Trade> findByOrderId(int64_t order_id);
    std::vector<Trade> findByTraderId(int64_t trader_id, int limit = 100);
    std::vector<Trade> findAll(int limit = 100);
    int64_t insert(const Trade& trade);

private:
    DatabaseManager& db_;
    Trade mapRowToTrade(Statement& stmt);
};

} // namespace tradeshield
