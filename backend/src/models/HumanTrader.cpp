#include "HumanTrader.hpp"

namespace tradeshield {

HumanTrader::HumanTrader(int64_t id, std::string name, int64_t cash_balance_paise, std::string created_at)
    : Trader(id, std::move(name), cash_balance_paise, std::move(created_at)) {}

} // namespace tradeshield
