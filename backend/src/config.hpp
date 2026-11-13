#pragma once

#include <string>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace tradeshield {

struct AppConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    std::string db_path = "database/tradeshield.db";
    std::string environment = "development";
    std::string version = "1.0.0";
    std::string cors_origin = "http://localhost:5173";

    static AppConfig loadFromEnv() {
        AppConfig cfg;

        // Port resolution: Render PORT has highest priority, falling back to TRADESHIELD_PORT
        const char* port_env = std::getenv("PORT");
        if (!port_env || port_env[0] == '\0') {
            port_env = std::getenv("TRADESHIELD_PORT");
        }
        if (port_env && port_env[0] != '\0') {
            try {
                char* endptr = nullptr;
                long parsed_port = std::strtol(port_env, &endptr, 10);
                if (endptr != port_env && *endptr == '\0' && parsed_port >= 1 && parsed_port <= 65535) {
                    cfg.port = static_cast<uint16_t>(parsed_port);
                } else {
                    std::cerr << "[CONFIG WARN] Invalid PORT environment variable value: '" 
                              << port_env << "'. Using default port " << cfg.port << std::endl;
                }
            } catch (...) {
                std::cerr << "[CONFIG WARN] Failed to parse PORT environment variable. Using default " 
                          << cfg.port << std::endl;
            }
        }

        if (const char* h = std::getenv("TRADESHIELD_HOST")) {
            if (h[0] != '\0') {
                cfg.host = h;
            }
        }
        if (const char* d = std::getenv("TRADESHIELD_DB_PATH")) {
            if (d[0] != '\0') {
                cfg.db_path = d;
            }
        }
        if (const char* e = std::getenv("TRADESHIELD_ENV")) {
            if (e[0] != '\0') {
                cfg.environment = e;
            }
        }
        if (const char* c = std::getenv("TRADESHIELD_CORS_ORIGIN")) {
            if (c[0] != '\0') {
                cfg.cors_origin = c;
            }
        }
        return cfg;
    }
};

} // namespace tradeshield
