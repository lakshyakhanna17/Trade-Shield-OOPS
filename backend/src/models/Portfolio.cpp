#include "Portfolio.hpp"

namespace tradeshield {

Portfolio::Portfolio(int64_t portfolio_id, int64_t trader_id,
                     int64_t total_value_paise, std::string updated_at)
    : portfolio_id_(portfolio_id), trader_id_(trader_id),
      total_value_paise_(total_value_paise), updated_at_(std::move(updated_at)) {
    if (trader_id_ <= 0) {
        throw ValidationException("Invalid trader ID in portfolio");
    }
    if (total_value_paise_ < 0) {
        throw ValidationException("Portfolio total value cannot be negative");
    }
}

void Portfolio::updateTotalValue(int64_t new_total_value_paise, const std::string& updated_at) {
    if (new_total_value_paise < 0) {
        throw ValidationException("Portfolio total value cannot be negative");
    }
    total_value_paise_ = new_total_value_paise;
    updated_at_ = updated_at;
}

} // namespace tradeshield
