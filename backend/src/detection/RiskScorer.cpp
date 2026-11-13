#include "RiskScorer.hpp"
#include <algorithm>
#include <unordered_set>

namespace tradeshield {

int RiskScorer::calculateScore(const std::vector<DetectionResult>& results) {
    int max_score = 0;
    int sum_others = 0;

    for (const auto& res : results) {
        if (!res.triggered) {
            continue;
        }

        if (res.risk_points > max_score) {
            sum_others += max_score;
            max_score = res.risk_points;
        } else {
            sum_others += res.risk_points;
        }
    }

    if (max_score == 0 && sum_others == 0) {
        return 0;
    }

    int composite = max_score + static_cast<int>(sum_others * 0.25);
    return std::clamp(composite, 0, 100);
}

int RiskScorer::calculateScoreFromAlerts(const std::vector<Alert>& alerts) {
    int max_score = 0;
    int sum_others = 0;

    for (const auto& alert : alerts) {
        // Cleared alerts are considered resolved/false-positive
        if (alert.getReviewStatus() == AlertReviewStatus::CLEARED) {
            continue;
        }

        int pts = alert.getRiskPoints();
        if (pts > max_score) {
            sum_others += max_score;
            max_score = pts;
        } else {
            sum_others += pts;
        }
    }

    if (max_score == 0 && sum_others == 0) {
        return 0;
    }

    int composite = max_score + static_cast<int>(sum_others * 0.25);
    return std::clamp(composite, 0, 100);
}

std::string RiskScorer::classifySeverity(int score) {
    if (score < 25) {
        return "LOW";
    }
    if (score < 50) {
        return "MEDIUM";
    }
    if (score < 75) {
        return "HIGH";
    }
    return "CRITICAL";
}

TraderRiskProfile RiskScorer::buildProfile(int64_t trader_id,
                                          const std::string& trader_name,
                                          const std::vector<Alert>& alerts) {
    TraderRiskProfile profile;
    profile.trader_id = trader_id;
    profile.trader_name = trader_name;
    profile.total_alerts = static_cast<int>(alerts.size());

    std::unordered_set<std::string> unique_rules;

    for (const auto& a : alerts) {
        switch (a.getSeverity()) {
            case AlertSeverity::LOW:
                profile.low_severity_alerts++;
                break;
            case AlertSeverity::MEDIUM:
                profile.medium_severity_alerts++;
                break;
            case AlertSeverity::HIGH:
                profile.high_severity_alerts++;
                break;
        }

        if (a.getReviewStatus() == AlertReviewStatus::OPEN) {
            profile.unreviewed_alerts++;
        }

        if (profile.last_alert_time.empty() || a.getCreatedAt() > profile.last_alert_time) {
            profile.last_alert_time = a.getCreatedAt();
        }

        unique_rules.insert(a.getRuleName());
    }

    for (const auto& r : unique_rules) {
        profile.triggered_rules.push_back(r);
    }
    std::sort(profile.triggered_rules.begin(), profile.triggered_rules.end());

    profile.overall_risk_score = calculateScoreFromAlerts(alerts);
    profile.risk_tier = classifySeverity(profile.overall_risk_score);

    return profile;
}

} // namespace tradeshield
