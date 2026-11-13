#include "crow.h"
#include "crow/middlewares/cors.h"
#include <nlohmann/json.hpp>
#include "config.hpp"
#include "database/DatabaseManager.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include "database/repositories/PortfolioRepository.hpp"
#include "database/repositories/AlertRepository.hpp"
#include "detection/FraudEngine.hpp"
#include "engine/TradingEngine.hpp"
#include "simulation/SimulationManager.hpp"
#include "api/TradingController.hpp"
#include "api/SimulationController.hpp"
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

int main() {
    tradeshield::AppConfig config = tradeshield::AppConfig::loadFromEnv();

    std::cout << "=====================================================" << std::endl;
    std::cout << "             TradeShield Server (C++17)             " << std::endl;
    std::cout << "=====================================================" << std::endl;

    // Ensure database directory exists
    std::filesystem::path db_file_path(config.db_path);
    if (db_file_path.has_parent_path()) {
        std::filesystem::create_directories(db_file_path.parent_path());
    }

    // Initialize Database and Repositories
    tradeshield::DatabaseManager db(config.db_path);

    // Initialize schema and seed data if database is empty
    try {
        auto check_stmt = db.prepare("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='traders';");
        bool table_exists = false;
        if (check_stmt.step()) {
            table_exists = (check_stmt.getInt(0) > 0);
        }

        if (!table_exists) {
            std::cout << "[DB INIT] Initializing schema and seed data into " << config.db_path << std::endl;
            if (std::filesystem::exists("database/schema.sql")) {
                db.initializeSchemaFromFile("database/schema.sql");
            }
            if (std::filesystem::exists("database/seed.sql")) {
                db.loadSeedDataFromFile("database/seed.sql");
            }
            std::cout << "[DB INIT] Schema and seed data successfully initialized." << std::endl;
        }

        // Migrate alerts table to ensure incident aggregation columns exist
        auto cols_stmt = db.prepare("PRAGMA table_info(alerts);");
        bool has_first_seen = false;
        while (cols_stmt.step()) {
            if (cols_stmt.getText(1) == "first_seen_at") {
                has_first_seen = true;
                break;
            }
        }
        if (!has_first_seen) {
            std::cout << "[DB MIGRATION] Adding incident aggregation columns to alerts..." << std::endl;
            db.execute("ALTER TABLE alerts ADD COLUMN first_seen_at TEXT;");
            db.execute("ALTER TABLE alerts ADD COLUMN last_seen_at TEXT;");
            db.execute("ALTER TABLE alerts ADD COLUMN occurrence_count INTEGER NOT NULL DEFAULT 1;");
            db.execute("ALTER TABLE alerts ADD COLUMN latest_order_id INTEGER;");
            db.execute("UPDATE alerts SET first_seen_at = created_at WHERE first_seen_at IS NULL;");
            db.execute("UPDATE alerts SET last_seen_at = created_at WHERE last_seen_at IS NULL;");
            db.execute("UPDATE alerts SET latest_order_id = order_id WHERE latest_order_id IS NULL AND order_id IS NOT NULL;");
            db.execute("CREATE INDEX IF NOT EXISTS idx_alerts_trader_rule_status ON alerts(trader_id, rule_name, review_status);");
            std::cout << "[DB MIGRATION] Migration successfully applied." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB WARN] Schema check/init warning: " << e.what() << std::endl;
    }

    tradeshield::TraderRepository trader_repo(db);
    tradeshield::StockRepository stock_repo(db);
    tradeshield::OrderRepository order_repo(db);
    tradeshield::TradeRepository trade_repo(db);
    tradeshield::PositionRepository position_repo(db);
    tradeshield::PortfolioRepository portfolio_repo(db);
    tradeshield::AlertRepository alert_repo(db);

    // Initialize Shield Fraud & Anomaly Detection Engine
    auto fraud_engine = std::make_shared<tradeshield::FraudEngine>(
        alert_repo, trader_repo, order_repo, trade_repo
    );

    // Initialize Trading Engine and link with Fraud Engine
    tradeshield::TradingEngine trading_engine(
        db, trader_repo, stock_repo, order_repo, trade_repo, position_repo, portfolio_repo
    );
    trading_engine.setFraudEngine(fraud_engine);

    // Initialize Bot Simulation Manager
    tradeshield::SimulationManager sim_manager(trading_engine, trader_repo);

    // Crow application configured with global CORS middleware
    crow::App<crow::CORSHandler> app;

    // Configure CORS globally using the configured origin
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin(config.cors_origin)
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)
        .headers("Content-Type", "Authorization");

    // GET /api/health - Minimal health check endpoint
    CROW_ROUTE(app, "/api/health")
    ([&config]() {
        json response = {
            {"status", "healthy"},
            {"service", "TradeShield Backend API"},
            {"version", config.version},
            {"environment", config.environment}
        };

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // Register all Trading and Shield Fraud REST API routes
    tradeshield::TradingController::registerRoutes(app, trading_engine, alert_repo, *fraud_engine);

    // Register Simulation Control REST API routes
    tradeshield::SimulationController::registerRoutes(app, sim_manager);

    std::cout << "TradeShield server starting on " << config.host << ":" << config.port << std::endl;
    std::cout << "Configured CORS origin: " << config.cors_origin << std::endl;
    std::cout << "Endpoints registered: /api/health, /api/orders, /api/trades, /api/portfolio, /api/positions, /api/stocks, /api/traders, /api/alerts, /api/risk, /api/simulation/start, /api/simulation/stop, /api/simulation/status, /api/simulation/reset" << std::endl;

    app.bindaddr(config.host).port(config.port).multithreaded().run();

    return 0;
}
