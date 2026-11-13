#include "BaseDetectionRule.hpp"
#include <cstdio>
#include <ctime>
#include <cmath>

namespace tradeshield {

BaseDetectionRule::BaseDetectionRule(std::string rule_id, std::string rule_name, std::string description)
    : rule_id_(std::move(rule_id)),
      rule_name_(std::move(rule_name)),
      description_(std::move(description)) {}

int64_t BaseDetectionRule::parseIsoTimestamp(const std::string& iso_ts) {
    if (iso_ts.empty()) {
        return 0;
    }
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (std::sscanf(iso_ts.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
                    &year, &month, &day, &hour, &minute, &second) >= 6) {
        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        tm.tm_isdst = 0;
#if defined(_WIN32) || defined(_WIN64)
        return static_cast<int64_t>(_mkgmtime(&tm));
#else
        return static_cast<int64_t>(timegm(&tm));
#endif
    }
    return 0;
}

int64_t BaseDetectionRule::getSecondsBetween(const std::string& earlier_ts, const std::string& later_ts) {
    int64_t t1 = parseIsoTimestamp(earlier_ts);
    int64_t t2 = parseIsoTimestamp(later_ts);
    return t2 >= t1 ? (t2 - t1) : (t1 - t2);
}

} // namespace tradeshield
