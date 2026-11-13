#pragma once

#include "TradingEvent.hpp"
#include "models/Order.hpp"
#include "models/Trade.hpp"
#include <vector>
#include <string>

namespace tradeshield {

/**
 * @brief Aggregated context supplied to detection rules for evaluation.
 * Contains the triggering event, trader identity, and chronological recent activity.
 */
struct DetectionContext {
    int64_t trader_id{0};
    std::string trader_name;
    std::string trader_type; // "HUMAN" or "BOT"
    int64_t cash_balance_paise{0};
    TradingEvent current_event;
    std::vector<Order> recent_orders;
    std::vector<Trade> recent_trades;
    int window_seconds{60};
};

} // namespace tradeshield
