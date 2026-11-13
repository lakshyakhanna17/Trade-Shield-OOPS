#pragma once

#include "Trader.hpp"

namespace tradeshield {

/**
 * @brief Represents a human trader interacting via the frontend.
 * Inherits from Trader.
 */
class HumanTrader : public Trader {
public:
    HumanTrader(int64_t id, std::string name, int64_t cash_balance_paise, std::string created_at);
    ~HumanTrader() override = default;

    std::string getTraderType() const override { return "HUMAN"; }
    bool isBot() const override { return false; }
    std::optional<std::string> getBotStrategy() const override { return std::nullopt; }
    std::string getRoleDescription() const override { return "Human Manual Trader"; }
};

} // namespace tradeshield
