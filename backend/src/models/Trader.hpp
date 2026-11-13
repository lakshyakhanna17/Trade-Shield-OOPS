#pragma once

#include "Types.hpp"
#include <string>
#include <optional>
#include <cstdint>
#include <limits>

namespace tradeshield {

/**
 * @brief Abstract base class representing a market participant (Human or Bot).
 * Demonstrates Abstraction, Encapsulation, and Polymorphism.
 */
class Trader {
public:
    Trader(int64_t id, std::string name, int64_t cash_balance_paise, std::string created_at);
    virtual ~Trader() = default;

    // Encapsulated getters
    int64_t getId() const noexcept { return id_; }
    const std::string& getName() const noexcept { return name_; }
    int64_t getCashBalancePaise() const noexcept { return cash_balance_paise_; }
    const std::string& getCreatedAt() const noexcept { return created_at_; }

    // Controlled financial mutations ensuring invariant: cash_balance_paise >= 0
    void creditCash(int64_t paise);
    void debitCash(int64_t paise);
    void setCashBalancePaise(int64_t paise);

    // Polymorphic interface methods
    virtual std::string getTraderType() const = 0;
    virtual bool isBot() const = 0;
    virtual std::optional<std::string> getBotStrategy() const = 0;
    virtual std::string getRoleDescription() const = 0;

protected:
    int64_t id_{0};
    std::string name_;
    int64_t cash_balance_paise_{0};
    std::string created_at_;
};

} // namespace tradeshield
