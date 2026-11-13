#pragma once

#include <string>
#include <stdexcept>
#include <optional>
#include <cstdint>
#include <ostream>

namespace tradeshield {

// Exception thrown on domain model or financial validation failures
class ValidationException : public std::invalid_argument {
public:
    explicit ValidationException(const std::string& message)
        : std::invalid_argument(message) {}
};

enum class OrderSide {
    BUY,
    SELL
};

inline std::string orderSideToString(OrderSide side) {
    switch (side) {
        case OrderSide::BUY: return "BUY";
        case OrderSide::SELL: return "SELL";
    }
    return "UNKNOWN";
}

inline OrderSide stringToOrderSide(const std::string& str) {
    if (str == "BUY") return OrderSide::BUY;
    if (str == "SELL") return OrderSide::SELL;
    throw ValidationException("Invalid order side: " + str);
}

inline std::ostream& operator<<(std::ostream& os, OrderSide side) {
    return os << orderSideToString(side);
}

enum class OrderStatus {
    PENDING,
    PARTIALLY_FILLED,
    EXECUTED,
    CANCELLED,
    REJECTED
};

inline std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::EXECUTED: return "EXECUTED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
    }
    return "UNKNOWN";
}

inline OrderStatus stringToOrderStatus(const std::string& str) {
    if (str == "PENDING") return OrderStatus::PENDING;
    if (str == "PARTIALLY_FILLED") return OrderStatus::PARTIALLY_FILLED;
    if (str == "EXECUTED") return OrderStatus::EXECUTED;
    if (str == "CANCELLED") return OrderStatus::CANCELLED;
    if (str == "REJECTED") return OrderStatus::REJECTED;
    throw ValidationException("Invalid order status: " + str);
}

inline std::ostream& operator<<(std::ostream& os, OrderStatus status) {
    return os << orderStatusToString(status);
}

enum class AlertSeverity {
    LOW,
    MEDIUM,
    HIGH
};

inline std::string alertSeverityToString(AlertSeverity sev) {
    switch (sev) {
        case AlertSeverity::LOW: return "LOW";
        case AlertSeverity::MEDIUM: return "MEDIUM";
        case AlertSeverity::HIGH: return "HIGH";
    }
    return "UNKNOWN";
}

inline AlertSeverity stringToAlertSeverity(const std::string& str) {
    if (str == "LOW") return AlertSeverity::LOW;
    if (str == "MEDIUM") return AlertSeverity::MEDIUM;
    if (str == "HIGH") return AlertSeverity::HIGH;
    throw ValidationException("Invalid alert severity: " + str);
}

inline std::ostream& operator<<(std::ostream& os, AlertSeverity sev) {
    return os << alertSeverityToString(sev);
}

enum class AlertReviewStatus {
    OPEN,
    REVIEWED,
    CLEARED
};

inline std::string alertReviewStatusToString(AlertReviewStatus status) {
    switch (status) {
        case AlertReviewStatus::OPEN: return "OPEN";
        case AlertReviewStatus::REVIEWED: return "REVIEWED";
        case AlertReviewStatus::CLEARED: return "CLEARED";
    }
    return "UNKNOWN";
}

inline AlertReviewStatus stringToAlertReviewStatus(const std::string& str) {
    if (str == "OPEN") return AlertReviewStatus::OPEN;
    if (str == "REVIEWED") return AlertReviewStatus::REVIEWED;
    if (str == "CLEARED") return AlertReviewStatus::CLEARED;
    throw ValidationException("Invalid alert review status: " + str);
}

inline std::ostream& operator<<(std::ostream& os, AlertReviewStatus status) {
    return os << alertReviewStatusToString(status);
}

} // namespace tradeshield
