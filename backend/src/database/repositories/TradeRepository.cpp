#include "TradeRepository.hpp"

namespace tradeshield {

Trade TradeRepository::mapRowToTrade(Statement& stmt) {
    int64_t trade_id = stmt.getInt64(0);
    int64_t order_id = stmt.getInt64(1);
    int64_t quantity = stmt.getInt64(2);
    int64_t price = stmt.getInt64(3);
    std::string executed_at = stmt.getText(4);

    return Trade(trade_id, order_id, quantity, price, std::move(executed_at));
}

std::optional<Trade> TradeRepository::findById(int64_t trade_id) {
    auto stmt = db_.prepare(
        "SELECT trade_id, order_id, executed_quantity, executed_price_paise, executed_at "
        "FROM trades WHERE trade_id = ?;"
    );
    stmt.bindInt64(1, trade_id);
    if (stmt.step()) {
        return mapRowToTrade(stmt);
    }
    return std::nullopt;
}

std::vector<Trade> TradeRepository::findByOrderId(int64_t order_id) {
    std::vector<Trade> trades;
    auto stmt = db_.prepare(
        "SELECT trade_id, order_id, executed_quantity, executed_price_paise, executed_at "
        "FROM trades WHERE order_id = ? ORDER BY executed_at ASC;"
    );
    stmt.bindInt64(1, order_id);
    while (stmt.step()) {
        trades.push_back(mapRowToTrade(stmt));
    }
    return trades;
}

std::vector<Trade> TradeRepository::findByTraderId(int64_t trader_id, int limit) {
    std::vector<Trade> trades;
    auto stmt = db_.prepare(
        "SELECT t.trade_id, t.order_id, t.executed_quantity, t.executed_price_paise, t.executed_at "
        "FROM trades t "
        "JOIN orders o ON t.order_id = o.order_id "
        "WHERE o.trader_id = ? "
        "ORDER BY t.trade_id DESC LIMIT ?;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt(2, limit);
    while (stmt.step()) {
        trades.push_back(mapRowToTrade(stmt));
    }
    return trades;
}

std::vector<Trade> TradeRepository::findAll(int limit) {
    std::vector<Trade> trades;
    auto stmt = db_.prepare(
        "SELECT trade_id, order_id, executed_quantity, executed_price_paise, executed_at "
        "FROM trades ORDER BY trade_id DESC LIMIT ?;"
    );
    stmt.bindInt(1, limit);
    while (stmt.step()) {
        trades.push_back(mapRowToTrade(stmt));
    }
    return trades;
}

int64_t TradeRepository::insert(const Trade& trade) {
    auto stmt = db_.prepare(
        "INSERT INTO trades (order_id, executed_quantity, executed_price_paise, executed_at) "
        "VALUES (?, ?, ?, ?);"
    );
    stmt.bindInt64(1, trade.getOrderId());
    stmt.bindInt64(2, trade.getExecutedQuantity());
    stmt.bindInt64(3, trade.getExecutedPricePaise());
    stmt.bindText(4, trade.getExecutedAt());

    stmt.execute();
    return db_.getLastInsertRowId();
}

} // namespace tradeshield
