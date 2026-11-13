#include "OrderRepository.hpp"

namespace tradeshield {

Order OrderRepository::mapRowToOrder(Statement& stmt) {
    int64_t id = stmt.getInt64(0);
    int64_t trader_id = stmt.getInt64(1);
    int64_t stock_id = stmt.getInt64(2);
    OrderSide side = stringToOrderSide(stmt.getText(3));
    int64_t quantity = stmt.getInt64(4);
    int64_t limit_price = stmt.getInt64(5);
    OrderStatus status = stringToOrderStatus(stmt.getText(6));
    std::string created_at = stmt.getText(7);
    std::string updated_at = stmt.getText(8);

    return Order(id, trader_id, stock_id, side, quantity, limit_price, status, std::move(created_at), std::move(updated_at));
}

std::optional<Order> OrderRepository::findById(int64_t order_id) {
    auto stmt = db_.prepare(
        "SELECT order_id, trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at "
        "FROM orders WHERE order_id = ?;"
    );
    stmt.bindInt64(1, order_id);
    if (stmt.step()) {
        return mapRowToOrder(stmt);
    }
    return std::nullopt;
}

std::vector<Order> OrderRepository::findByTraderId(int64_t trader_id, int limit) {
    std::vector<Order> orders;
    auto stmt = db_.prepare(
        "SELECT order_id, trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at "
        "FROM orders WHERE trader_id = ? ORDER BY created_at DESC LIMIT ?;"
    );
    stmt.bindInt64(1, trader_id);
    stmt.bindInt(2, limit);
    while (stmt.step()) {
        orders.push_back(mapRowToOrder(stmt));
    }
    return orders;
}

std::vector<Order> OrderRepository::findByStockId(int64_t stock_id, int limit) {
    std::vector<Order> orders;
    auto stmt = db_.prepare(
        "SELECT order_id, trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at "
        "FROM orders WHERE stock_id = ? ORDER BY created_at DESC LIMIT ?;"
    );
    stmt.bindInt64(1, stock_id);
    stmt.bindInt(2, limit);
    while (stmt.step()) {
        orders.push_back(mapRowToOrder(stmt));
    }
    return orders;
}

std::vector<Order> OrderRepository::findByStatus(OrderStatus status, int limit) {
    std::vector<Order> orders;
    auto stmt = db_.prepare(
        "SELECT order_id, trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at "
        "FROM orders WHERE status = ? ORDER BY created_at DESC LIMIT ?;"
    );
    stmt.bindText(1, orderStatusToString(status));
    stmt.bindInt(2, limit);
    while (stmt.step()) {
        orders.push_back(mapRowToOrder(stmt));
    }
    return orders;
}

std::vector<Order> OrderRepository::findAll(int limit) {
    std::vector<Order> orders;
    auto stmt = db_.prepare(
        "SELECT order_id, trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at "
        "FROM orders ORDER BY order_id DESC LIMIT ?;"
    );
    stmt.bindInt(1, limit);
    while (stmt.step()) {
        orders.push_back(mapRowToOrder(stmt));
    }
    return orders;
}

int64_t OrderRepository::insert(const Order& order) {
    auto stmt = db_.prepare(
        "INSERT INTO orders (trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
    );
    stmt.bindInt64(1, order.getTraderId());
    stmt.bindInt64(2, order.getStockId());
    stmt.bindText(3, orderSideToString(order.getSide()));
    stmt.bindInt64(4, order.getQuantity());
    stmt.bindInt64(5, order.getLimitPricePaise());
    stmt.bindText(6, orderStatusToString(order.getStatus()));
    stmt.bindText(7, order.getCreatedAt());
    stmt.bindText(8, order.getUpdatedAt());

    stmt.execute();
    return db_.getLastInsertRowId();
}

bool OrderRepository::updateStatus(int64_t order_id, OrderStatus new_status, const std::string& updated_at) {
    auto stmt = db_.prepare(
        "UPDATE orders SET status = ?, updated_at = ? WHERE order_id = ?;"
    );
    stmt.bindText(1, orderStatusToString(new_status));
    stmt.bindText(2, updated_at);
    stmt.bindInt64(3, order_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

} // namespace tradeshield
