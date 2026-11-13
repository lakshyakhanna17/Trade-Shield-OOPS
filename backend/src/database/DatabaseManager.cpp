#include "DatabaseManager.hpp"
#include <fstream>
#include <sstream>

namespace tradeshield {

DatabaseManager::DatabaseManager(const std::string& db_path)
    : db_path_(db_path) {
    int rc = sqlite3_open_v2(
        db_path_.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "Failed to allocate SQLite memory";
        if (db_) {
            sqlite3_close_v2(db_);
            db_ = nullptr;
        }
        throw DatabaseException("sqlite3_open_v2 failed for " + db_path_, err, rc);
    }

    configurePragmas();
}

DatabaseManager::~DatabaseManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_transaction_ && db_) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

DatabaseManager::DatabaseManager(DatabaseManager&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    db_ = other.db_;
    db_path_ = std::move(other.db_path_);
    in_transaction_ = other.in_transaction_;
    other.db_ = nullptr;
    other.in_transaction_ = false;
}

DatabaseManager& DatabaseManager::operator=(DatabaseManager&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        if (db_) {
            sqlite3_close_v2(db_);
        }
        db_ = other.db_;
        db_path_ = std::move(other.db_path_);
        in_transaction_ = other.in_transaction_;
        other.db_ = nullptr;
        other.in_transaction_ = false;
    }
    return *this;
}

void DatabaseManager::configurePragmas() {
    // Enforce foreign keys (requirement in TradeShield Database Schema doc)
    execute("PRAGMA foreign_keys = ON;");

    // Enable WAL mode for high-concurrency read/write operations (unless in-memory db)
    if (db_path_ != ":memory:" && db_path_.find("mode=memory") == std::string::npos) {
        execute("PRAGMA journal_mode = WAL;");
        execute("PRAGMA synchronous = NORMAL;");
    }

    // Set busy timeout to 5000ms
    execute("PRAGMA busy_timeout = 5000;");

    // Ensure portfolio_snapshots table and index exist for historical tracking
    execute(
        "CREATE TABLE IF NOT EXISTS portfolio_snapshots ("
        "    snapshot_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,"
        "    total_value_paise INTEGER NOT NULL CHECK (total_value_paise >= 0),"
        "    cash_balance_paise INTEGER NOT NULL CHECK (cash_balance_paise >= 0),"
        "    holdings_value_paise INTEGER NOT NULL CHECK (holdings_value_paise >= 0),"
        "    snapshot_time TEXT NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_portfolio_snapshots_trader_time ON portfolio_snapshots(trader_id, snapshot_time);"
    );
}

void DatabaseManager::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        throw DatabaseException("sqlite3_exec failed for [" + sql + "]", err, rc);
    }
}

void DatabaseManager::executeScript(const std::string& sql_script) {
    execute(sql_script);
}

void DatabaseManager::initializeSchemaFromFile(const std::string& schema_filepath) {
    std::ifstream file(schema_filepath);
    if (!file.is_open()) {
        throw DatabaseException("Cannot open schema file: " + schema_filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    executeScript(buffer.str());
}

void DatabaseManager::loadSeedDataFromFile(const std::string& seed_filepath) {
    std::ifstream file(seed_filepath);
    if (!file.is_open()) {
        throw DatabaseException("Cannot open seed file: " + seed_filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    executeScript(buffer.str());
}

Statement DatabaseManager::prepare(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    return Statement(db_, sql);
}

int64_t DatabaseManager::getLastInsertRowId() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int64_t>(sqlite3_last_insert_rowid(db_));
}

void DatabaseManager::beginTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_transaction_) {
        throw DatabaseException("Transaction already in progress on this connection");
    }
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        throw DatabaseException("BEGIN TRANSACTION failed", err, rc);
    }
    in_transaction_ = true;
}

void DatabaseManager::commit() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!in_transaction_) {
        throw DatabaseException("No active transaction to commit");
    }
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, "COMMIT TRANSACTION;", nullptr, nullptr, &err_msg);
    in_transaction_ = false;
    if (rc != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        throw DatabaseException("COMMIT TRANSACTION failed", err, rc);
    }
}

void DatabaseManager::rollback() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!in_transaction_) {
        return; // Safe to no-op
    }
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, "ROLLBACK TRANSACTION;", nullptr, nullptr, &err_msg);
    in_transaction_ = false;
    if (rc != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        throw DatabaseException("ROLLBACK TRANSACTION failed", err, rc);
    }
}

TransactionGuard DatabaseManager::createTransaction() {
    return TransactionGuard(*this);
}

// TransactionGuard Implementation
TransactionGuard::TransactionGuard(DatabaseManager& db)
    : db_(&db) {
    db_->beginTransaction();
}

TransactionGuard::~TransactionGuard() {
    if (!committed_ && !rolled_back_ && db_) {
        try {
            db_->rollback();
        } catch (...) {
            // Suppress exceptions in destructors
        }
    }
}

TransactionGuard::TransactionGuard(TransactionGuard&& other) noexcept
    : db_(other.db_), committed_(other.committed_), rolled_back_(other.rolled_back_) {
    other.db_ = nullptr;
}

TransactionGuard& TransactionGuard::operator=(TransactionGuard&& other) noexcept {
    if (this != &other) {
        if (!committed_ && !rolled_back_ && db_) {
            try {
                db_->rollback();
            } catch (...) {}
        }
        db_ = other.db_;
        committed_ = other.committed_;
        rolled_back_ = other.rolled_back_;
        other.db_ = nullptr;
    }
    return *this;
}

void TransactionGuard::commit() {
    if (committed_) {
        throw DatabaseException("TransactionGuard::commit called more than once");
    }
    if (rolled_back_) {
        throw DatabaseException("Cannot commit transaction that has already been rolled back");
    }
    db_->commit();
    committed_ = true;
}

void TransactionGuard::rollback() {
    if (committed_) {
        throw DatabaseException("Cannot rollback transaction that has already been committed");
    }
    if (!rolled_back_ && db_) {
        db_->rollback();
        rolled_back_ = true;
    }
}

} // namespace tradeshield
