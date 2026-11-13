#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Trader.hpp"
#include "models/HumanTrader.hpp"
#include "models/BotTrader.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace tradeshield {

/**
 * @brief Repository for persisting and retrieving Trader instances.
 * Returns polymorphic std::unique_ptr<Trader> (HumanTrader or BotTrader).
 */
class TraderRepository {
public:
    explicit TraderRepository(DatabaseManager& db) : db_(db) {}

    std::optional<std::unique_ptr<Trader>> findById(int64_t trader_id);
    std::vector<std::unique_ptr<Trader>> findAll();
    int64_t insert(const Trader& trader);
    bool updateCashBalance(int64_t trader_id, int64_t new_cash_paise);
    bool deleteById(int64_t trader_id);

private:
    DatabaseManager& db_;
    std::unique_ptr<Trader> mapRowToTrader(Statement& stmt);
};

} // namespace tradeshield
