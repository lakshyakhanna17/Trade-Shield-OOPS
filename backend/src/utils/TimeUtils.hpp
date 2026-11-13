#pragma once

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace tradeshield {

/**
 * @brief Utility for formatting current UTC time into standard ISO-8601 strings.
 */
class TimeUtils {
public:
    static std::string currentIsoTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm gmt_time{};
#if defined(_WIN32) || defined(_WIN64)
        gmtime_s(&gmt_time, &in_time_t);
#else
        gmtime_r(&in_time_t, &gmt_time);
#endif
        std::stringstream ss;
        ss << std::put_time(&gmt_time, "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }
};

} // namespace tradeshield
