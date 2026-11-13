#include "Trade.hpp"
#include <limits>

namespace tradeshield {

Trade::Trade(int64_t trade_id, int64_t order_id, int64_t executed_quantity,
             int64_t executed_price_paise, std::string executed_at)
    : trade_id_(trade_id), order_id_(order_id), executed_quantity_(executed_quantity),
      executed_price_paise_(executed_price_paise), executed_at_(std::move(executed_at)) {
    if (order_id_ <= 0) {
        throw ValidationException("Invalid order ID for trade execution");
    }
    if (executed_quantity_ <= 0) {
        throw ValidationException("Executed trade quantity must be strictly positive");
    }
    if (executed_price_paise_ <= 0) {
        throw ValidationException("Executed trade price must be strictly positive");
    }
}

int64_t Trade::getTradeValuePaise() const {
    if (executed_quantity_ > 0 && executed_price_paise_ > std::numeric_limits<int64_t>::max() / executed_quantity_) {
        throw ValidationException("Trade execution value exceeds maximum allowable integer");
    }
    return executed_quantity_ * executed_price_paise_;
}

} // namespace tradeshield
