#pragma once

#include "IDetectionRule.hpp"
#include <string>

namespace tradeshield {

/**
 * @brief Common base class providing rule metadata and timestamp arithmetic utilities.
 */
class BaseDetectionRule : public IDetectionRule {
public:
    BaseDetectionRule(std::string rule_id, std::string rule_name, std::string description);
    ~BaseDetectionRule() override = default;

    std::string getRuleId() const override { return rule_id_; }
    std::string getRuleName() const override { return rule_name_; }
    std::string getDescription() const override { return description_; }

    // Helper: parses ISO-8601 timestamp string into epoch seconds
    static int64_t parseIsoTimestamp(const std::string& iso_ts);

    // Helper: returns the elapsed seconds between two ISO-8601 timestamps
    static int64_t getSecondsBetween(const std::string& earlier_ts, const std::string& later_ts);

protected:
    std::string rule_id_;
    std::string rule_name_;
    std::string description_;
};

} // namespace tradeshield
