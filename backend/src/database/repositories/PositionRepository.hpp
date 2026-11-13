#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Position.hpp"
#include <vector>
#include <optional>

namespace tradeshield {

/**
 * @brief Repository for persisting and querying aggregate stock Positions.
 */
class PositionRepository {
public:
    explicit PositionRepository(DatabaseManager& db) : db_(db) {}

    std::optional<Position> findByTraderAndStock(int64_t trader_id, int64_t stock_id);
    std::vector<Position> findByTraderId(int64_t trader_id);
    int64_t saveOrUpdate(const Position& position);
    bool deleteByTraderAndStock(int64_t trader_id, int64_t stock_id);

private:
    DatabaseManager& db_;
    Position mapRowToPosition(Statement& stmt);
};

} // namespace tradeshield
