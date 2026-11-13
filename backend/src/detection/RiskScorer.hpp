#pragma once

#include "DetectionResult.hpp"
#include "models/Alert.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace tradeshield {

/**
 * @brief Summary risk profile for a trader, aggregating fraud detection metrics.
 */
struct TraderRiskProfile {
    int64_t trader_id{0};
    std::string trader_name;
    int overall_risk_score{0}; // 0 - 100
    std::string risk_tier;     // "LOW", "MEDIUM", "HIGH", "CRITICAL"
    int total_alerts{0};
    int low_severity_alerts{0};
    int medium_severity_alerts{0};
    int high_severity_alerts{0};
    int unreviewed_alerts{0};
    std::string last_alert_time;
    std::vector<std::string> triggered_rules;
};

/**
 * @brief Computes composite risk scores (0-100) and risk tiers from detection events and alerts.
 */
class RiskScorer {
public:
    /**
     * @brief Computes composite score from a batch of detection results.
     * Formula: max_rule_score + (sum_of_other_scores * 0.25), clamped to [0, 100].
     */
    static int calculateScore(const std::vector<DetectionResult>& results);

    /**
     * @brief Computes composite score from a trader's historical alerts.
     * Only OPEN and REVIEWED alerts contribute; CLEARED alerts contribute 0.
     */
    static int calculateScoreFromAlerts(const std::vector<Alert>& alerts);

    /**
     * @brief Maps integer score (0-100) to severity classification band.
     * 0-24: LOW, 25-49: MEDIUM, 50-74: HIGH, 75-100: CRITICAL.
     */
    static std::string classifySeverity(int score);

    /**
     * @brief Builds a complete risk profile for a trader given their alerts.
     */
    static TraderRiskProfile buildProfile(int64_t trader_id,
                                         const std::string& trader_name,
                                         const std::vector<Alert>& alerts);
};

} // namespace tradeshield
