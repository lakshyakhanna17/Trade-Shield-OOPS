#include "Stock.hpp"

namespace tradeshield {

Stock::Stock(int64_t stock_id, std::string symbol, std::string company_name,
             std::string exchange, int64_t current_price_paise, std::string updated_at)
    : stock_id_(stock_id), symbol_(std::move(symbol)), company_name_(std::move(company_name)),
      exchange_(std::move(exchange)), current_price_paise_(current_price_paise), updated_at_(std::move(updated_at)) {
    if (symbol_.empty()) {
        throw ValidationException("Stock ticker symbol cannot be empty");
    }
    if (company_name_.empty()) {
        throw ValidationException("Stock company name cannot be empty");
    }
    if (exchange_.empty()) {
        throw ValidationException("Stock exchange cannot be empty");
    }
    if (current_price_paise_ <= 0) {
        throw ValidationException("Stock price must be strictly positive (greater than 0 paise)");
    }
}

void Stock::updatePrice(int64_t new_price_paise, const std::string& updated_at) {
    if (new_price_paise <= 0) {
        throw ValidationException("Updated stock price must be strictly positive");
    }
    current_price_paise_ = new_price_paise;
    updated_at_ = updated_at;
}

} // namespace tradeshield
