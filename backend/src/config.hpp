#pragma once

#include <string>
#include <cstdint>
#include <cstdlib>

namespace tradeshield {

struct AppConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    std::string db_path = "database/tradeshield.db";
    std::string environment = "development";
    std::string version = "1.0.0";

    static AppConfig loadFromEnv() {
        AppConfig cfg;
        if (const char* p = std::getenv("TRADESHIELD_PORT")) {
            cfg.port = static_cast<uint16_t>(std::atoi(p));
        }
        if (const char* h = std::getenv("TRADESHIELD_HOST")) {
            cfg.host = h;
        }
        if (const char* d = std::getenv("TRADESHIELD_DB_PATH")) {
            cfg.db_path = d;
        }
        if (const char* e = std::getenv("TRADESHIELD_ENV")) {
            cfg.environment = e;
        }
        return cfg;
    }
};

} // namespace tradeshield
