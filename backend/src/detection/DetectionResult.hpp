#pragma once

#include "models/Types.hpp"
#include <string>
#include <map>

namespace tradeshield {

/**
 * @brief Structured outcome produced by a detection rule evaluation.
 */
struct DetectionResult {
    std::string rule_id;
    std::string rule_name;
    bool triggered{false};
    int risk_points{0}; // Constrained to 0–100
    AlertSeverity severity{AlertSeverity::LOW};
    std::string description;
    std::map<std::string, std::string> evidence;

    static DetectionResult notTriggered(std::string id, std::string name, std::string desc = "") {
        return {std::move(id), std::move(name), false, 0, AlertSeverity::LOW, std::move(desc), {}};
    }
};

} // namespace tradeshield
