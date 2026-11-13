#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Represents cached total portfolio valuation for a trader.
 */
class Portfolio {
public:
    Portfolio(int64_t portfolio_id, int64_t trader_id,
              int64_t total_value_paise, std::string updated_at);

    int64_t getId() const noexcept { return portfolio_id_; }
    int64_t getTraderId() const noexcept { return trader_id_; }
    int64_t getTotalValuePaise() const noexcept { return total_value_paise_; }
    const std::string& getUpdatedAt() const noexcept { return updated_at_; }

    void updateTotalValue(int64_t new_total_value_paise, const std::string& updated_at);

private:
    int64_t portfolio_id_{0};
    int64_t trader_id_{0};
    int64_t total_value_paise_{0};
    std::string updated_at_;
};

} // namespace tradeshield
