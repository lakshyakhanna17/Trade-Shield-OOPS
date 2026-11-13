#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Represents a financial equity instrument (e.g. NSE listed stock).
 * Encapsulates price and market metadata with strict validation.
 */
class Stock {
public:
    Stock(int64_t stock_id, std::string symbol, std::string company_name,
          std::string exchange, int64_t current_price_paise, std::string updated_at);

    int64_t getId() const noexcept { return stock_id_; }
    const std::string& getSymbol() const noexcept { return symbol_; }
    const std::string& getCompanyName() const noexcept { return company_name_; }
    const std::string& getExchange() const noexcept { return exchange_; }
    int64_t getCurrentPricePaise() const noexcept { return current_price_paise_; }
    const std::string& getUpdatedAt() const noexcept { return updated_at_; }

    void updatePrice(int64_t new_price_paise, const std::string& updated_at);

private:
    int64_t stock_id_{0};
    std::string symbol_;
    std::string company_name_;
    std::string exchange_;
    int64_t current_price_paise_{0};
    std::string updated_at_;
};

} // namespace tradeshield
