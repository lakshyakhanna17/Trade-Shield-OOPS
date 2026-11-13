#pragma once

#include "models/Types.hpp"
#include <string>
#include <cstdint>

namespace tradeshield {

enum class TradingEventType {
    ORDER_SUBMITTED,
    ORDER_EXECUTED,
    ORDER_CANCELLED
};

inline std::string tradingEventTypeToString(TradingEventType t) {
    switch (t) {
        case TradingEventType::ORDER_SUBMITTED: return "ORDER_SUBMITTED";
        case TradingEventType::ORDER_EXECUTED:  return "ORDER_EXECUTED";
        case TradingEventType::ORDER_CANCELLED: return "ORDER_CANCELLED";
    }
    return "UNKNOWN";
}

/**
 * @brief Represents a discrete trading event emitted by the trading engine.
 */
struct TradingEvent {
    TradingEventType type{TradingEventType::ORDER_SUBMITTED};
    int64_t trader_id{0};
    int64_t order_id{0};
    int64_t stock_id{0};
    OrderSide side{OrderSide::BUY};
    int64_t quantity{0};
    int64_t price_paise{0};
    std::string timestamp; // ISO-8601 UTC
};

} // namespace tradeshield
