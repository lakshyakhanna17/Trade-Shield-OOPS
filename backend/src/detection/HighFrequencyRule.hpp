#pragma once

#include "BaseDetectionRule.hpp"

namespace tradeshield {

/**
 * @brief Detection rule for unusually rapid order submission frequency.
 * Flags order bursts that exceed normal trading speed within a tight sliding window.
 */
class HighFrequencyRule : public BaseDetectionRule {
public:
    explicit HighFrequencyRule(int window_seconds = 10,
                               int burst_order_threshold = 4);

    DetectionResult evaluate(const DetectionContext& ctx) override;

    int getWindowSeconds() const noexcept { return window_seconds_; }
    void setWindowSeconds(int w) noexcept { window_seconds_ = w; }

    int getBurstOrderThreshold() const noexcept { return burst_order_threshold_; }
    void setBurstOrderThreshold(int t) noexcept { burst_order_threshold_ = t; }

private:
    int window_seconds_{10};
    int burst_order_threshold_{4};
};

} // namespace tradeshield
