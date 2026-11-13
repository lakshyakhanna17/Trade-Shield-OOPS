#pragma once

#include "DatabaseException.hpp"
#include <sqlite3.h>
#include <string>
#include <cstdint>
#include <optional>

namespace tradeshield {

/**
 * @brief RAII wrapper for SQLite prepared statement (sqlite3_stmt*).
 * Guarantees proper finalization and provides type-safe parameter binding and column reading.
 */
class Statement {
public:
    Statement(sqlite3* db, const std::string& sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    // Binding parameters (1-indexed per SQLite API)
    void bindInt(int index, int val);
    void bindInt64(int index, int64_t val);
    void bindText(int index, const std::string& val);
    void bindNull(int index);
    void bindOptionalInt64(int index, const std::optional<int64_t>& val);
    void bindOptionalText(int index, const std::optional<std::string>& val);

    // Step statement: returns true if SQLITE_ROW, false if SQLITE_DONE
    bool step();

    // Execute statement to completion (for INSERT, UPDATE, DELETE)
    void execute();

    // Reset statement for reuse
    void reset();

    // Reading columns (0-indexed per SQLite API)
    int getInt(int col) const;
    int64_t getInt64(int col) const;
    std::string getText(int col) const;
    bool isNull(int col) const;
    std::optional<int64_t> getOptionalInt64(int col) const;
    std::optional<std::string> getOptionalText(int col) const;

    sqlite3_stmt* getRaw() const noexcept { return stmt_; }

private:
    sqlite3* db_{nullptr};
    sqlite3_stmt* stmt_{nullptr};
    std::string sql_;

    void checkDbAndStmt() const;
};

} // namespace tradeshield
