#include "Trader.hpp"

namespace tradeshield {

Trader::Trader(int64_t id, std::string name, int64_t cash_balance_paise, std::string created_at)
    : id_(id), name_(std::move(name)), cash_balance_paise_(cash_balance_paise), created_at_(std::move(created_at)) {
    if (name_.empty()) {
        throw ValidationException("Trader name cannot be empty");
    }
    if (cash_balance_paise_ < 0) {
        throw ValidationException("Initial cash balance cannot be negative");
    }
}

void Trader::creditCash(int64_t paise) {
    if (paise <= 0) {
        throw ValidationException("Credit amount must be strictly positive");
    }
    // Prevent 64-bit integer overflow
    if (paise > std::numeric_limits<int64_t>::max() - cash_balance_paise_) {
        throw ValidationException("Credit amount causes integer overflow");
    }
    cash_balance_paise_ += paise;
}

void Trader::debitCash(int64_t paise) {
    if (paise <= 0) {
        throw ValidationException("Debit amount must be strictly positive");
    }
    if (cash_balance_paise_ < paise) {
        throw ValidationException("Insufficient cash balance: available " +
                                  std::to_string(cash_balance_paise_) + " paise, requested " +
                                  std::to_string(paise) + " paise");
    }
    cash_balance_paise_ -= paise;
}

void Trader::setCashBalancePaise(int64_t paise) {
    if (paise < 0) {
        throw ValidationException("Cash balance cannot be negative");
    }
    cash_balance_paise_ = paise;
}

} // namespace tradeshield
