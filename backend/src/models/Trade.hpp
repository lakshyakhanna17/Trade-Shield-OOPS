#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Represents an executed trade execution of an order.
 */
class Trade {
public:
    Trade(int64_t trade_id, int64_t order_id, int64_t executed_quantity,
          int64_t executed_price_paise, std::string executed_at);

    int64_t getId() const noexcept { return trade_id_; }
    int64_t getOrderId() const noexcept { return order_id_; }
    int64_t getExecutedQuantity() const noexcept { return executed_quantity_; }
    int64_t getExecutedPricePaise() const noexcept { return executed_price_paise_; }
    const std::string& getExecutedAt() const noexcept { return executed_at_; }

    int64_t getTradeValuePaise() const;

private:
    int64_t trade_id_{0};
    int64_t order_id_{0};
    int64_t executed_quantity_{0};
    int64_t executed_price_paise_{0};
    std::string executed_at_;
};

} // namespace tradeshield
