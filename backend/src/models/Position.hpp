#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Represents aggregate holdings of a specific stock owned by a trader.
 * Computes moving weighted average acquisition price and unrealized P&L.
 */
class Position {
public:
    Position(int64_t position_id, int64_t trader_id, int64_t stock_id,
             int64_t quantity, int64_t average_buy_price_paise);

    int64_t getId() const noexcept { return position_id_; }
    int64_t getTraderId() const noexcept { return trader_id_; }
    int64_t getStockId() const noexcept { return stock_id_; }
    int64_t getQuantity() const noexcept { return quantity_; }
    int64_t getAverageBuyPricePaise() const noexcept { return average_buy_price_paise_; }

    void addShares(int64_t shares, int64_t buy_price_paise);
    void deductShares(int64_t shares);

    int64_t getTotalInvestedPaise() const;
    int64_t getCurrentValuePaise(int64_t current_market_price_paise) const;
    int64_t getUnrealizedPnlPaise(int64_t current_market_price_paise) const;

private:
    int64_t position_id_{0};
    int64_t trader_id_{0};
    int64_t stock_id_{0};
    int64_t quantity_{0};
    int64_t average_buy_price_paise_{0};
};

} // namespace tradeshield
