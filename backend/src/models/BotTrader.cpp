#include "BotTrader.hpp"

namespace tradeshield {

void BotTrader::validateStrategy(const std::string& strategy) {
    if (strategy != "NORMAL" &&
        strategy != "HIGH_VOLUME" &&
        strategy != "RAPID_CANCEL" &&
        strategy != "RANDOM") {
        throw ValidationException("Invalid bot strategy: " + strategy);
    }
}

BotTrader::BotTrader(int64_t id, std::string name, std::string bot_strategy, int64_t cash_balance_paise, std::string created_at)
    : Trader(id, std::move(name), cash_balance_paise, std::move(created_at)),
      bot_strategy_(std::move(bot_strategy)) {
    validateStrategy(bot_strategy_);
}

void BotTrader::setBotStrategy(const std::string& strategy) {
    validateStrategy(strategy);
    bot_strategy_ = strategy;
}

} // namespace tradeshield
