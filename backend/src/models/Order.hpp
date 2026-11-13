#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Represents an order submitted by a trader.
 * Implements strict status lifecycle transitions and order validation.
 */
class Order {
public:
    Order(int64_t order_id, int64_t trader_id, int64_t stock_id, OrderSide side,
          int64_t quantity, int64_t limit_price_paise, OrderStatus status,
          std::string created_at, std::string updated_at);

    // Getters
    int64_t getId() const noexcept { return order_id_; }
    int64_t getTraderId() const noexcept { return trader_id_; }
    int64_t getStockId() const noexcept { return stock_id_; }
    OrderSide getSide() const noexcept { return side_; }
    int64_t getQuantity() const noexcept { return quantity_; }
    int64_t getLimitPricePaise() const noexcept { return limit_price_paise_; }
    OrderStatus getStatus() const noexcept { return status_; }
    const std::string& getCreatedAt() const noexcept { return created_at_; }
    const std::string& getUpdatedAt() const noexcept { return updated_at_; }

    // Computes order total nominal value (quantity * limit_price) with overflow check
    int64_t getTotalValuePaise() const;

    // Order status lifecycle methods
    bool isTerminal() const noexcept;
    void execute(const std::string& timestamp);
    void partiallyFill(const std::string& timestamp);
    void cancel(const std::string& timestamp);
    void reject(const std::string& timestamp);

private:
    int64_t order_id_{0};
    int64_t trader_id_{0};
    int64_t stock_id_{0};
    OrderSide side_{OrderSide::BUY};
    int64_t quantity_{0};
    int64_t limit_price_paise_{0};
    OrderStatus status_{OrderStatus::PENDING};
    std::string created_at_;
    std::string updated_at_;

    void ensureNonTerminal(const std::string& action) const;
};

} // namespace tradeshield
