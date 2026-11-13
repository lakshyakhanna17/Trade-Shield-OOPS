#pragma once

#include "DetectionContext.hpp"
#include "DetectionResult.hpp"
#include <string>

namespace tradeshield {

/**
 * @brief Abstract interface representing a fraud/anomaly detection rule.
 * Demonstrates Abstraction and Polymorphism in the detection subsystem.
 */
class IDetectionRule {
public:
    virtual ~IDetectionRule() = default;

    virtual std::string getRuleId() const = 0;
    virtual std::string getRuleName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual DetectionResult evaluate(const DetectionContext& ctx) = 0;
};

} // namespace tradeshield
