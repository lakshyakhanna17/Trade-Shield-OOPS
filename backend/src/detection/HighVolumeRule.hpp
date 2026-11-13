#pragma once

#include "BaseDetectionRule.hpp"

namespace tradeshield {

/**
 * @brief Detection rule for unusually large order volume or repeated large orders.
 * Follows the core rule: a single large order contributes low risk; repeated large
 * orders or volume spikes over historical average trigger higher severity.
 */
class HighVolumeRule : public BaseDetectionRule {
public:
    explicit HighVolumeRule(int64_t quantity_threshold = 100,
                            int64_t value_threshold_paise = 5000000,
                            double spike_multiplier = 3.0);

    DetectionResult evaluate(const DetectionContext& ctx) override;

    // Configuration getters & setters
    int64_t getQuantityThreshold() const noexcept { return quantity_threshold_; }
    void setQuantityThreshold(int64_t q) noexcept { quantity_threshold_ = q; }

    int64_t getValueThresholdPaise() const noexcept { return value_threshold_paise_; }
    void setValueThresholdPaise(int64_t v) noexcept { value_threshold_paise_ = v; }

    double getSpikeMultiplier() const noexcept { return spike_multiplier_; }
    void setSpikeMultiplier(double m) noexcept { spike_multiplier_ = m; }

private:
    int64_t quantity_threshold_{100};
    int64_t value_threshold_paise_{5000000}; // Rs 50,000.00
    double spike_multiplier_{3.0};
};

} // namespace tradeshield
