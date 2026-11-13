#include "Position.hpp"
#include <limits>

namespace tradeshield {

Position::Position(int64_t position_id, int64_t trader_id, int64_t stock_id,
                   int64_t quantity, int64_t average_buy_price_paise)
    : position_id_(position_id), trader_id_(trader_id), stock_id_(stock_id),
      quantity_(quantity), average_buy_price_paise_(average_buy_price_paise) {
    if (trader_id_ <= 0) {
        throw ValidationException("Invalid trader ID in position");
    }
    if (stock_id_ <= 0) {
        throw ValidationException("Invalid stock ID in position");
    }
    if (quantity_ < 0) {
        throw ValidationException("Position quantity cannot be negative");
    }
    if (average_buy_price_paise_ < 0) {
        throw ValidationException("Average buy price cannot be negative");
    }
}

void Position::addShares(int64_t shares, int64_t buy_price_paise) {
    if (shares <= 0) {
        throw ValidationException("Shares to add must be strictly positive");
    }
    if (buy_price_paise <= 0) {
        throw ValidationException("Buy price must be strictly positive");
    }

    // Check overflow: (quantity_ * average_buy_price_paise_) + (shares * buy_price_paise)
    if (quantity_ > 0 && average_buy_price_paise_ > std::numeric_limits<int64_t>::max() / quantity_) {
        throw ValidationException("Existing position value overflow");
    }
    if (buy_price_paise > std::numeric_limits<int64_t>::max() / shares) {
        throw ValidationException("Additional position value overflow");
    }

    int64_t existing_cost = quantity_ * average_buy_price_paise_;
    int64_t additional_cost = shares * buy_price_paise;

    if (additional_cost > std::numeric_limits<int64_t>::max() - existing_cost) {
        throw ValidationException("Combined total position cost overflow");
    }

    int64_t total_cost = existing_cost + additional_cost;
    int64_t new_quantity = quantity_ + shares;

    quantity_ = new_quantity;
    average_buy_price_paise_ = total_cost / new_quantity;
}

void Position::deductShares(int64_t shares) {
    if (shares <= 0) {
        throw ValidationException("Shares to deduct must be strictly positive");
    }
    if (shares > quantity_) {
        throw ValidationException("Cannot sell more shares than currently held: held " +
                                  std::to_string(quantity_) + ", requested " + std::to_string(shares));
    }

    quantity_ -= shares;
    if (quantity_ == 0) {
        average_buy_price_paise_ = 0;
    }
}

int64_t Position::getTotalInvestedPaise() const {
    if (quantity_ > 0 && average_buy_price_paise_ > std::numeric_limits<int64_t>::max() / quantity_) {
        throw ValidationException("Total invested amount causes integer overflow");
    }
    return quantity_ * average_buy_price_paise_;
}

int64_t Position::getCurrentValuePaise(int64_t current_market_price_paise) const {
    if (current_market_price_paise < 0) {
        throw ValidationException("Market price cannot be negative");
    }
    if (quantity_ > 0 && current_market_price_paise > std::numeric_limits<int64_t>::max() / quantity_) {
        throw ValidationException("Position market value causes integer overflow");
    }
    return quantity_ * current_market_price_paise;
}

int64_t Position::getUnrealizedPnlPaise(int64_t current_market_price_paise) const {
    return getCurrentValuePaise(current_market_price_paise) - getTotalInvestedPaise();
}

} // namespace tradeshield
