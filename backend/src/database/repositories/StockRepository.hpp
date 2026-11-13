#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Stock.hpp"
#include <vector>
#include <optional>

namespace tradeshield {

/**
 * @brief Repository for persisting and retrieving Stock market instruments.
 */
class StockRepository {
public:
    explicit StockRepository(DatabaseManager& db) : db_(db) {}

    std::optional<Stock> findById(int64_t stock_id);
    std::optional<Stock> findBySymbol(const std::string& symbol);
    std::vector<Stock> findAll();
    int64_t insert(const Stock& stock);
    bool updatePrice(int64_t stock_id, int64_t new_price_paise, const std::string& updated_at);

private:
    DatabaseManager& db_;
    Stock mapRowToStock(Statement& stmt);
};

} // namespace tradeshield
