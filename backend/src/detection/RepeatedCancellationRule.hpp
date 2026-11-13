#pragma once

#include "BaseDetectionRule.hpp"

namespace tradeshield {

/**
 * @brief Detection rule for repeated order cancellations and excessive cancel ratios.
 * Helps identify spoofing, layering, or phantom liquidity patterns.
 */
class RepeatedCancellationRule : public BaseDetectionRule {
public:
    explicit RepeatedCancellationRule(int window_seconds = 60,
                                      int min_cancellations_threshold = 3,
                                      double cancellation_ratio_threshold = 0.5);

    DetectionResult evaluate(const DetectionContext& ctx) override;

    int getWindowSeconds() const noexcept { return window_seconds_; }
    void setWindowSeconds(int w) noexcept { window_seconds_ = w; }

    int getMinCancellationsThreshold() const noexcept { return min_cancellations_threshold_; }
    void setMinCancellationsThreshold(int c) noexcept { min_cancellations_threshold_ = c; }

    double getCancellationRatioThreshold() const noexcept { return cancellation_ratio_threshold_; }
    void setCancellationRatioThreshold(double r) noexcept { cancellation_ratio_threshold_ = r; }

private:
    int window_seconds_{60};
    int min_cancellations_threshold_{3};
    double cancellation_ratio_threshold_{0.5};
};

} // namespace tradeshield
