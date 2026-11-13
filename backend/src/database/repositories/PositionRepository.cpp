#include "PositionRepository.hpp"

namespace tradeshield {

Position PositionRepository::mapRowToPosition(Statement& stmt) {
    int64_t position_id = stmt.getInt64(0);
    int64_t trader_id = stmt.getInt64(1);
    int64_t stock_id = stmt.getInt64(2);
    int64_t quantity = stmt.getInt64(3);
    int64_t average_buy_price = stmt.getInt64(4);

    return Position(position_id, trader_id, stock_id, quantity, average_buy_price);
}

std::optional<Position> PositionRepository::findByTraderAndStock(int64_t trader_id, int64_t stock_id) {
    auto stmt = db_.prepare(
        "SELECT position_id, trader_id, stock_id, quantity, average_buy_price_paise "
        "FROM positions WHERE trader_id = ? AND stock_id = ?;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt64(2, stock_id);
    if (stmt.step()) {
        return mapRowToPosition(stmt);
    }
    return std::nullopt;
}

std::vector<Position> PositionRepository::findByTraderId(int64_t trader_id) {
    std::vector<Position> positions;
    auto stmt = db_.prepare(
        "SELECT position_id, trader_id, stock_id, quantity, average_buy_price_paise "
        "FROM positions WHERE trader_id = ? ORDER BY stock_id ASC;"
    );
    stmt.bindInt64(1, trader_id);
    while (stmt.step()) {
        positions.push_back(mapRowToPosition(stmt));
    }
    return positions;
}

int64_t PositionRepository::saveOrUpdate(const Position& position) {
    auto stmt = db_.prepare(
        "INSERT INTO positions (trader_id, stock_id, quantity, average_buy_price_paise) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT(trader_id, stock_id) DO UPDATE SET "
        "quantity = excluded.quantity, "
        "average_buy_price_paise = excluded.average_buy_price_paise;"
    );
    stmt.bindInt64(1, position.getTraderId());
    stmt.bindInt64(2, position.getStockId());
    stmt.bindInt64(3, position.getQuantity());
    stmt.bindInt64(4, position.getAverageBuyPricePaise());

    stmt.execute();
    return db_.getLastInsertRowId();
}

bool PositionRepository::deleteByTraderAndStock(int64_t trader_id, int64_t stock_id) {
    auto stmt = db_.prepare(
        "DELETE FROM positions WHERE trader_id = ? AND stock_id = ?;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt64(2, stock_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

} // namespace tradeshield
