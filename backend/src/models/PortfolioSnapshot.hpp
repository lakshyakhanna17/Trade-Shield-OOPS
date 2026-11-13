#pragma once

#include "Types.hpp"
#include <string>
#include <cstdint>
#include <utility>

namespace tradeshield {

/**
 * @brief Represents an immutable point-in-time portfolio valuation snapshot persisted in SQLite.
 */
class PortfolioSnapshot {
public:
    PortfolioSnapshot(int64_t snapshot_id, int64_t trader_id,
                      int64_t total_value_paise, int64_t cash_balance_paise,
                      int64_t holdings_value_paise, std::string snapshot_time)
        : snapshot_id_(snapshot_id),
          trader_id_(trader_id),
          total_value_paise_(total_value_paise),
          cash_balance_paise_(cash_balance_paise),
          holdings_value_paise_(holdings_value_paise),
          snapshot_time_(std::move(snapshot_time)) {
        if (total_value_paise_ < 0) {
            throw ValidationException("Snapshot total value cannot be negative");
        }
        if (cash_balance_paise_ < 0) {
            throw ValidationException("Snapshot cash balance cannot be negative");
        }
        if (holdings_value_paise_ < 0) {
            throw ValidationException("Snapshot holdings value cannot be negative");
        }
    }

    int64_t getId() const noexcept { return snapshot_id_; }
    int64_t getTraderId() const noexcept { return trader_id_; }
    int64_t getTotalValuePaise() const noexcept { return total_value_paise_; }
    int64_t getCashBalancePaise() const noexcept { return cash_balance_paise_; }
    int64_t getHoldingsValuePaise() const noexcept { return holdings_value_paise_; }
    const std::string& getSnapshotTime() const noexcept { return snapshot_time_; }

private:
    int64_t snapshot_id_{0};
    int64_t trader_id_{0};
    int64_t total_value_paise_{0};
    int64_t cash_balance_paise_{0};
    int64_t holdings_value_paise_{0};
    std::string snapshot_time_;
};

} // namespace tradeshield
