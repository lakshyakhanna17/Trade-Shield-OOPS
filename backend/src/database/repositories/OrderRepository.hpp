#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Order.hpp"
#include <vector>
#include <optional>

namespace tradeshield {

/**
 * @brief Repository for persisting and querying Orders.
 */
class OrderRepository {
public:
    explicit OrderRepository(DatabaseManager& db) : db_(db) {}

    std::optional<Order> findById(int64_t order_id);
    std::vector<Order> findByTraderId(int64_t trader_id, int limit = 100);
    std::vector<Order> findByStockId(int64_t stock_id, int limit = 100);
    std::vector<Order> findByStatus(OrderStatus status, int limit = 100);
    std::vector<Order> findAll(int limit = 100);
    int64_t insert(const Order& order);
    bool updateStatus(int64_t order_id, OrderStatus new_status, const std::string& updated_at);

private:
    DatabaseManager& db_;
    Order mapRowToOrder(Statement& stmt);
};

} // namespace tradeshield
