#pragma once

#include "engine/TradingEngine.hpp"
#include <string>
#include <optional>

namespace tradeshield {

/**
 * @brief Categorization of actions performed during a simulation tick.
 */
enum class BotActionType {
    NONE,
    BUY_ORDER,
    SELL_ORDER,
    CANCEL_ORDER
};

inline std::string botActionTypeToString(BotActionType t) {
    switch (t) {
        case BotActionType::BUY_ORDER: return "BUY_ORDER";
        case BotActionType::SELL_ORDER: return "SELL_ORDER";
        case BotActionType::CANCEL_ORDER: return "CANCEL_ORDER";
        case BotActionType::NONE: return "NONE";
    }
    return "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& os, BotActionType t) {
    return os << botActionTypeToString(t);
}

/**
 * @brief Output result produced by an automated bot action.
 */
struct BotActionResult {
    BotActionType action_type{BotActionType::NONE};
    bool success{false};
    std::string message;
    std::optional<Order> order;
    std::optional<Trade> trade;
};

/**
 * @brief Abstract interface defining the bot algorithmic trading strategy (Strategy Pattern).
 * Implements runtime polymorphism enabling independent strategy definitions without
 * modifying SimulationManager or TradingEngine.
 */
class IBotStrategy {
public:
    virtual ~IBotStrategy() = default;

    // Strategy identification
    virtual std::string getStrategyName() const = 0;

    // Executes a discrete algorithmic action using the central TradingEngine
    virtual BotActionResult executeTick(TradingEngine& engine, int64_t bot_trader_id) = 0;
};

} // namespace tradeshield
