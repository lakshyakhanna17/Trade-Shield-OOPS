#pragma once

#include "IBotStrategy.hpp"
#include <cstdint>

namespace tradeshield {

/**
 * @brief Strategy simulating rapid quote spoofing and cancellation behavior.
 * Submits non-marketable limit orders that remain PENDING, then systematically
 * cancels them to trigger the Shield RepeatedCancellationRule.
 */
class RapidCancelBotStrategy : public IBotStrategy {
public:
    explicit RapidCancelBotStrategy(double price_discount_factor = 0.90);
    ~RapidCancelBotStrategy() override = default;

    std::string getStrategyName() const override { return "RAPID_CANCEL"; }

    BotActionResult executeTick(TradingEngine& engine, int64_t bot_trader_id) override;

    // Helper for generating rapid burst pairs (place non-marketable + cancel)
    BotActionResult executeBurstPair(TradingEngine& engine, int64_t bot_trader_id);

private:
    double price_discount_factor_{0.90};
    uint64_t tick_counter_{0};
};

} // namespace tradeshield
