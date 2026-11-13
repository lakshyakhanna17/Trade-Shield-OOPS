#pragma once

#include "IDetectionRule.hpp"
#include "TradingEvent.hpp"
#include "RiskScorer.hpp"
#include "database/repositories/AlertRepository.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <mutex>

namespace tradeshield {

/**
 * @brief Shield Fraud & Anomaly Detection Engine coordinator.
 * Manages detection rules, processes trading events, computes risk scores,
 * and persists triggered alerts without disrupting the trading transaction flow.
 */
class FraudEngine {
public:
    FraudEngine(AlertRepository& alert_repo,
                TraderRepository& trader_repo,
                OrderRepository& order_repo,
                TradeRepository& trade_repo);

    virtual ~FraudEngine() = default;

    // Registers default rule set: HighVolumeRule, RepeatedCancellationRule, HighFrequencyRule
    void registerDefaultRules();

    // Register a detection rule polymorphically
    void addRule(std::unique_ptr<IDetectionRule> rule);

    // Clear all registered rules (useful for test harnesses)
    void clearRules();

    // Get count of registered rules
    size_t getRuleCount() const noexcept;

    // Process a trading event through all registered rules.
    // Thread-safe and exception-safe.
    std::vector<Alert> processEvent(const TradingEvent& event);

    // Retrieve full risk profile for a single trader
    std::optional<TraderRiskProfile> getTraderRisk(int64_t trader_id);

    // Retrieve risk profiles for all traders
    std::vector<TraderRiskProfile> getAllTradersRisk();

    // Set episode window for incident aggregation (seconds)
    void setEpisodeWindowSeconds(int seconds) noexcept { episode_window_seconds_ = seconds; }
    int getEpisodeWindowSeconds() const noexcept { return episode_window_seconds_; }

private:
    AlertRepository& alert_repo_;
    TraderRepository& trader_repo_;
    OrderRepository& order_repo_;
    TradeRepository& trade_repo_;

    std::vector<std::unique_ptr<IDetectionRule>> rules_;
    int episode_window_seconds_{120};
    mutable std::mutex engine_mutex_;
};

} // namespace tradeshield
