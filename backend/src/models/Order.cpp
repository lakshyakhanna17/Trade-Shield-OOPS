#include "Order.hpp"
#include <limits>

namespace tradeshield {

Order::Order(int64_t order_id, int64_t trader_id, int64_t stock_id, OrderSide side,
             int64_t quantity, int64_t limit_price_paise, OrderStatus status,
             std::string created_at, std::string updated_at)
    : order_id_(order_id), trader_id_(trader_id), stock_id_(stock_id), side_(side),
      quantity_(quantity), limit_price_paise_(limit_price_paise), status_(status),
      created_at_(std::move(created_at)), updated_at_(std::move(updated_at)) {
    if (trader_id_ <= 0) {
        throw ValidationException("Invalid trader ID in order");
    }
    if (stock_id_ <= 0) {
        throw ValidationException("Invalid stock ID in order");
    }
    if (quantity_ <= 0) {
        throw ValidationException("Order quantity must be strictly positive");
    }
    if (limit_price_paise_ <= 0) {
        throw ValidationException("Order limit price must be strictly positive");
    }
}

int64_t Order::getTotalValuePaise() const {
    if (quantity_ > 0 && limit_price_paise_ > std::numeric_limits<int64_t>::max() / quantity_) {
        throw ValidationException("Order nominal value exceeds maximum allowable integer");
    }
    return quantity_ * limit_price_paise_;
}

bool Order::isTerminal() const noexcept {
    return status_ == OrderStatus::EXECUTED ||
           status_ == OrderStatus::CANCELLED ||
           status_ == OrderStatus::REJECTED;
}

void Order::ensureNonTerminal(const std::string& action) const {
    if (isTerminal()) {
        throw ValidationException("Cannot " + action + " order #" + std::to_string(order_id_) +
                                  " because it is in terminal state " + orderStatusToString(status_));
    }
}

void Order::execute(const std::string& timestamp) {
    ensureNonTerminal("execute");
    status_ = OrderStatus::EXECUTED;
    updated_at_ = timestamp;
}

void Order::partiallyFill(const std::string& timestamp) {
    ensureNonTerminal("partially fill");
    status_ = OrderStatus::PARTIALLY_FILLED;
    updated_at_ = timestamp;
}

void Order::cancel(const std::string& timestamp) {
    ensureNonTerminal("cancel");
    status_ = OrderStatus::CANCELLED;
    updated_at_ = timestamp;
}

void Order::reject(const std::string& timestamp) {
    ensureNonTerminal("reject");
    status_ = OrderStatus::REJECTED;
    updated_at_ = timestamp;
}

} // namespace tradeshield
