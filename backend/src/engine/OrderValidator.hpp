#pragma once

#include "models/Order.hpp"
#include "models/Trader.hpp"
#include "models/Stock.hpp"
#include "models/Position.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include <string>
#include <optional>

namespace tradeshield {

/**
 * @brief Represents the outcome of an order validation check.
 */
struct ValidationResult {
    bool is_valid{true};
    std::string error_message;

    static ValidationResult success() {
        return {true, ""};
    }

    static ValidationResult failure(std::string msg) {
        return {false, std::move(msg)};
    }
};

/**
 * @brief Independent business validation service for incoming and active orders.
 * Encapsulates order integrity, asset availability, and state eligibility.
 */
class OrderValidator {
public:
    OrderValidator(TraderRepository& trader_repo,
                   StockRepository& stock_repo,
                   PositionRepository& position_repo);

    /**
     * @brief Validates trader existence, stock existence, asset sufficiency, and value boundaries.
     */
    ValidationResult validateNewOrder(int64_t trader_id,
                                      int64_t stock_id,
                                      OrderSide side,
                                      int64_t quantity,
                                      int64_t limit_price_paise) const;

    /**
     * @brief Validates whether an order can be cancelled.
     */
    ValidationResult validateCancellation(int64_t order_id, const Order& order) const;

private:
    TraderRepository& trader_repo_;
    StockRepository& stock_repo_;
    PositionRepository& position_repo_;
};

} // namespace tradeshield
