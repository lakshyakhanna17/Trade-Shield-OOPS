#pragma once

#include <stdexcept>
#include <string>

namespace tradeshield {

/**
 * @brief Exception thrown on SQLite database operational errors.
 */
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message)
        : std::runtime_error(message) {}

    DatabaseException(const std::string& operation, const std::string& sqlite_err, int err_code = 0)
        : std::runtime_error(operation + " failed (code " + std::to_string(err_code) + "): " + sqlite_err),
          error_code_(err_code) {}

    int getErrorCode() const noexcept { return error_code_; }

private:
    int error_code_{0};
};

} // namespace tradeshield
