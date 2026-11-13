#pragma once

#include "DatabaseException.hpp"
#include "Statement.hpp"
#include <sqlite3.h>
#include <string>
#include <mutex>
#include <memory>
#include <cstdint>

namespace tradeshield {

class TransactionGuard;

/**
 * @brief Thread-safe RAII SQLite connection manager.
 * Configures foreign key enforcement, WAL mode, transaction management, and schema initialization.
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& db_path);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    DatabaseManager(DatabaseManager&& other) noexcept;
    DatabaseManager& operator=(DatabaseManager&& other) noexcept;

    // Execution utilities
    void execute(const std::string& sql);
    void executeScript(const std::string& sql_script);
    void initializeSchemaFromFile(const std::string& schema_filepath);
    void loadSeedDataFromFile(const std::string& seed_filepath);

    // Creates an RAII prepared statement
    Statement prepare(const std::string& sql);

    // Retrieve last inserted row id
    int64_t getLastInsertRowId() const noexcept;

    // Transaction management
    void beginTransaction();
    void commit();
    void rollback();

    // Factory method for RAII transaction guard
    TransactionGuard createTransaction();

    sqlite3* getRawDb() const noexcept { return db_; }
    std::mutex& getMutex() const noexcept { return mutex_; }

private:
    sqlite3* db_{nullptr};
    std::string db_path_;
    mutable std::mutex mutex_;
    bool in_transaction_{false};

    void configurePragmas();
};

/**
 * @brief RAII Transaction Guard ensuring atomic transactions.
 * Calls rollback() in destructor if commit() has not been explicitly called.
 */
class TransactionGuard {
public:
    explicit TransactionGuard(DatabaseManager& db);
    ~TransactionGuard();

    TransactionGuard(const TransactionGuard&) = delete;
    TransactionGuard& operator=(const TransactionGuard&) = delete;

    TransactionGuard(TransactionGuard&& other) noexcept;
    TransactionGuard& operator=(TransactionGuard&& other) noexcept;

    void commit();
    void rollback();

private:
    DatabaseManager* db_{nullptr};
    bool committed_{false};
    bool rolled_back_{false};
};

} // namespace tradeshield
