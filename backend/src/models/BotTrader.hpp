#pragma once

#include "Trader.hpp"

namespace tradeshield {

/**
 * @brief Represents an automated trading bot running an algorithmic strategy.
 * Inherits from Trader.
 */
class BotTrader : public Trader {
public:
    BotTrader(int64_t id, std::string name, std::string bot_strategy, int64_t cash_balance_paise, std::string created_at);
    ~BotTrader() override = default;

    std::string getTraderType() const override { return "BOT"; }
    bool isBot() const override { return true; }
    std::optional<std::string> getBotStrategy() const override { return bot_strategy_; }
    std::string getRoleDescription() const override { return "Automated Bot (" + bot_strategy_ + ")"; }

    void setBotStrategy(const std::string& strategy);

private:
    std::string bot_strategy_;
    static void validateStrategy(const std::string& strategy);
};

} // namespace tradeshield
