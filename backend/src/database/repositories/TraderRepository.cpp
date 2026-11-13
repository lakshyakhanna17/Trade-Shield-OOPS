#include "TraderRepository.hpp"

namespace tradeshield {

std::unique_ptr<Trader> TraderRepository::mapRowToTrader(Statement& stmt) {
    int64_t id = stmt.getInt64(0);
    std::string name = stmt.getText(1);
    std::string trader_type = stmt.getText(2);
    std::optional<std::string> bot_strategy = stmt.getOptionalText(3);
    int64_t cash = stmt.getInt64(4);
    std::string created_at = stmt.getText(5);

    if (trader_type == "HUMAN") {
        return std::make_unique<HumanTrader>(id, std::move(name), cash, std::move(created_at));
    }
    if (trader_type == "BOT") {
        std::string strategy = bot_strategy.value_or("NORMAL");
        return std::make_unique<BotTrader>(id, std::move(name), std::move(strategy), cash, std::move(created_at));
    }
    throw DatabaseException("Unknown trader_type in database: " + trader_type);
}

std::optional<std::unique_ptr<Trader>> TraderRepository::findById(int64_t trader_id) {
    auto stmt = db_.prepare(
        "SELECT trader_id, name, trader_type, bot_strategy, cash_balance_paise, created_at "
        "FROM traders WHERE trader_id = ?;"
    );
    stmt.bindInt64(1, trader_id);
    if (stmt.step()) {
        return mapRowToTrader(stmt);
    }
    return std::nullopt;
}

std::vector<std::unique_ptr<Trader>> TraderRepository::findAll() {
    std::vector<std::unique_ptr<Trader>> traders;
    auto stmt = db_.prepare(
        "SELECT trader_id, name, trader_type, bot_strategy, cash_balance_paise, created_at "
        "FROM traders ORDER BY trader_id ASC;"
    );
    while (stmt.step()) {
        traders.push_back(mapRowToTrader(stmt));
    }
    return traders;
}

int64_t TraderRepository::insert(const Trader& trader) {
    auto stmt = db_.prepare(
        "INSERT INTO traders (name, trader_type, bot_strategy, cash_balance_paise, created_at) "
        "VALUES (?, ?, ?, ?, ?);"
    );
    stmt.bindText(1, trader.getName());
    stmt.bindText(2, trader.getTraderType());
    stmt.bindOptionalText(3, trader.getBotStrategy());
    stmt.bindInt64(4, trader.getCashBalancePaise());
    stmt.bindText(5, trader.getCreatedAt());

    stmt.execute();
    return db_.getLastInsertRowId();
}

bool TraderRepository::updateCashBalance(int64_t trader_id, int64_t new_cash_paise) {
    if (new_cash_paise < 0) {
        throw ValidationException("Cannot update trader cash balance to negative amount");
    }
    auto stmt = db_.prepare(
        "UPDATE traders SET cash_balance_paise = ? WHERE trader_id = ?;"
    );
    stmt.bindInt64(1, new_cash_paise);
    stmt.bindInt64(2, trader_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

bool TraderRepository::deleteById(int64_t trader_id) {
    auto stmt = db_.prepare("DELETE FROM traders WHERE trader_id = ?;");
    stmt.bindInt64(1, trader_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

} // namespace tradeshield
