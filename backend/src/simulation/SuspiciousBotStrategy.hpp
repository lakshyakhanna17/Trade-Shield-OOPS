#pragma once

#include "IBotStrategy.hpp"
#include <cstdint>

namespace tradeshield {

/**
 * @brief Strategy simulating suspicious high-volume market manipulation behavior.
 * Generates abnormally large order quantities and consecutive volume spikes
 * specifically designed to trigger the Shield HighVolumeRule.
 */
class SuspiciousBotStrategy : public IBotStrategy {
public:
    explicit SuspiciousBotStrategy(int64_t target_quantity = 150);
    ~SuspiciousBotStrategy() override = default;

    std::string getStrategyName() const override { return "HIGH_VOLUME"; }

    BotActionResult executeTick(TradingEngine& engine, int64_t bot_trader_id) override;

    int64_t getTargetQuantity() const noexcept { return target_quantity_; }
    void setTargetQuantity(int64_t qty) noexcept { target_quantity_ = qty; }

private:
    int64_t target_quantity_{150};
    uint64_t tick_counter_{0};
};

} // namespace tradeshield
