#include "OrderValidator.hpp"
#include <limits>

namespace tradeshield {

OrderValidator::OrderValidator(TraderRepository& trader_repo,
                               StockRepository& stock_repo,
                               PositionRepository& position_repo)
    : trader_repo_(trader_repo),
      stock_repo_(stock_repo),
      position_repo_(position_repo) {}

ValidationResult OrderValidator::validateNewOrder(int64_t trader_id,
                                                  int64_t stock_id,
                                                  OrderSide side,
                                                  int64_t quantity,
                                                  int64_t limit_price_paise) const {
    if (quantity <= 0) {
        return ValidationResult::failure("Order quantity must be strictly positive (greater than 0)");
    }
    if (limit_price_paise <= 0) {
        return ValidationResult::failure("Order limit price must be strictly positive (greater than 0 paise)");
    }
    if (trader_id <= 0) {
        return ValidationResult::failure("Invalid trader ID");
    }
    if (stock_id <= 0) {
        return ValidationResult::failure("Invalid stock ID");
    }

    // Check for integer overflow when multiplying quantity * limit_price_paise
    if (quantity > 0 && limit_price_paise > std::numeric_limits<int64_t>::max() / quantity) {
        return ValidationResult::failure("Order nominal value exceeds maximum allowable financial integer");
    }
    int64_t nominal_value_paise = quantity * limit_price_paise;

    // Check trader existence
    auto trader_opt = trader_repo_.findById(trader_id);
    if (!trader_opt.has_value()) {
        return ValidationResult::failure("Trader not found: #" + std::to_string(trader_id));
    }

    // Check stock existence
    auto stock_opt = stock_repo_.findById(stock_id);
    if (!stock_opt.has_value()) {
        return ValidationResult::failure("Stock not found: #" + std::to_string(stock_id));
    }

    // Asset sufficiency checks
    if (side == OrderSide::BUY) {
        int64_t available_cash = trader_opt.value()->getCashBalancePaise();
        if (available_cash < nominal_value_paise) {
            return ValidationResult::failure("Insufficient cash balance: available " +
                                             std::to_string(available_cash) + " paise, required " +
                                             std::to_string(nominal_value_paise) + " paise");
        }
    } else if (side == OrderSide::SELL) {
        auto pos_opt = position_repo_.findByTraderAndStock(trader_id, stock_id);
        int64_t held_quantity = pos_opt.has_value() ? pos_opt->getQuantity() : 0;
        if (held_quantity < quantity) {
            return ValidationResult::failure("Insufficient stock holdings: currently holding " +
                                             std::to_string(held_quantity) + " shares of " +
                                             stock_opt->getSymbol() + ", requested to sell " +
                                             std::to_string(quantity) + " shares");
        }
    }

    return ValidationResult::success();
}

ValidationResult OrderValidator::validateCancellation(int64_t order_id, const Order& order) const {
    if (order.getId() != order_id) {
        return ValidationResult::failure("Order ID mismatch during cancellation validation");
    }
    if (order.getStatus() != OrderStatus::PENDING) {
        return ValidationResult::failure("Cannot cancel order #" + std::to_string(order_id) +
                                         " because it is in status " + orderStatusToString(order.getStatus()) +
                                         " (only PENDING orders can be cancelled)");
    }
    return ValidationResult::success();
}

} // namespace tradeshield
