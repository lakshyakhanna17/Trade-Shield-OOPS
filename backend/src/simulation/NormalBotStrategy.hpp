#pragma once

#include "IBotStrategy.hpp"
#include <cstdint>

namespace tradeshield {

/**
 * @brief Strategy simulating ordinary retail investor behavior.
 * Generates modest, bounded orders near market prices and occasional cancellations.
 * Designed to maintain a low risk score without triggering fraud alerts.
 */
class NormalBotStrategy : public IBotStrategy {
public:
    NormalBotStrategy() = default;
    ~NormalBotStrategy() override = default;

    std::string getStrategyName() const override { return "NORMAL"; }

    BotActionResult executeTick(TradingEngine& engine, int64_t bot_trader_id) override;

private:
    uint64_t tick_counter_{0};
};

} // namespace tradeshield
