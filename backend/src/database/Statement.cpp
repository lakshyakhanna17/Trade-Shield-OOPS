#include "Statement.hpp"

namespace tradeshield {

Statement::Statement(sqlite3* db, const std::string& sql)
    : db_(db), sql_(sql) {
    if (!db_) {
        throw DatabaseException("Cannot prepare statement with null database connection");
    }
    int rc = sqlite3_prepare_v2(db_, sql_.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseException("sqlite3_prepare_v2 failed for [" + sql_ + "]", sqlite3_errmsg(db_), rc);
    }
}

Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

Statement::Statement(Statement&& other) noexcept
    : db_(other.db_), stmt_(other.stmt_), sql_(std::move(other.sql_)) {
    other.db_ = nullptr;
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
        db_ = other.db_;
        stmt_ = other.stmt_;
        sql_ = std::move(other.sql_);
        other.db_ = nullptr;
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::checkDbAndStmt() const {
    if (!stmt_) {
        throw DatabaseException("Attempted operation on null or finalized prepared statement");
    }
}

void Statement::bindInt(int index, int val) {
    checkDbAndStmt();
    int rc = sqlite3_bind_int(stmt_, index, val);
    if (rc != SQLITE_OK) {
        throw DatabaseException("sqlite3_bind_int failed", sqlite3_errmsg(db_), rc);
    }
}

void Statement::bindInt64(int index, int64_t val) {
    checkDbAndStmt();
    int rc = sqlite3_bind_int64(stmt_, index, static_cast<sqlite3_int64>(val));
    if (rc != SQLITE_OK) {
        throw DatabaseException("sqlite3_bind_int64 failed", sqlite3_errmsg(db_), rc);
    }
}

void Statement::bindText(int index, const std::string& val) {
    checkDbAndStmt();
    int rc = sqlite3_bind_text(stmt_, index, val.c_str(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        throw DatabaseException("sqlite3_bind_text failed", sqlite3_errmsg(db_), rc);
    }
}

void Statement::bindNull(int index) {
    checkDbAndStmt();
    int rc = sqlite3_bind_null(stmt_, index);
    if (rc != SQLITE_OK) {
        throw DatabaseException("sqlite3_bind_null failed", sqlite3_errmsg(db_), rc);
    }
}

void Statement::bindOptionalInt64(int index, const std::optional<int64_t>& val) {
    if (val.has_value()) {
        bindInt64(index, val.value());
    } else {
        bindNull(index);
    }
}

void Statement::bindOptionalText(int index, const std::optional<std::string>& val) {
    if (val.has_value()) {
        bindText(index, val.value());
    } else {
        bindNull(index);
    }
}

bool Statement::step() {
    checkDbAndStmt();
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }
    throw DatabaseException("sqlite3_step failed for [" + sql_ + "]", sqlite3_errmsg(db_), rc);
}

void Statement::execute() {
    checkDbAndStmt();
    int rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw DatabaseException("sqlite3_step execution failed for [" + sql_ + "]", sqlite3_errmsg(db_), rc);
    }
}

void Statement::reset() {
    checkDbAndStmt();
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

int Statement::getInt(int col) const {
    checkDbAndStmt();
    return sqlite3_column_int(stmt_, col);
}

int64_t Statement::getInt64(int col) const {
    checkDbAndStmt();
    return static_cast<int64_t>(sqlite3_column_int64(stmt_, col));
}

std::string Statement::getText(int col) const {
    checkDbAndStmt();
    const unsigned char* text = sqlite3_column_text(stmt_, col);
    return text ? reinterpret_cast<const char*>(text) : "";
}

bool Statement::isNull(int col) const {
    checkDbAndStmt();
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}

std::optional<int64_t> Statement::getOptionalInt64(int col) const {
    if (isNull(col)) {
        return std::nullopt;
    }
    return getInt64(col);
}

std::optional<std::string> Statement::getOptionalText(int col) const {
    if (isNull(col)) {
        return std::nullopt;
    }
    return getText(col);
}

} // namespace tradeshield
