#pragma once

#include "IBotStrategy.hpp"
#include <random>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Strategy simulating pseudo-random trading activity within bounded limits.
 * Uses a PRNG engine with optional deterministic seed for reproducible testing.
 */
class RandomBotStrategy : public IBotStrategy {
public:
    explicit RandomBotStrategy(uint64_t seed = 0);
    ~RandomBotStrategy() override = default;

    std::string getStrategyName() const override { return "RANDOM"; }

    BotActionResult executeTick(TradingEngine& engine, int64_t bot_trader_id) override;

    void setSeed(uint64_t seed);

private:
    std::mt19937_64 rng_;
    uint64_t initial_seed_{0};
};

} // namespace tradeshield
