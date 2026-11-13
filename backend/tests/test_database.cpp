#include "TestFramework.hpp"
#include "database/DatabaseManager.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include "database/repositories/PortfolioRepository.hpp"
#include "database/repositories/AlertRepository.hpp"

using namespace tradeshield;

static const char* TEST_SCHEMA = R"(
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS traders (
    trader_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    trader_type TEXT NOT NULL CHECK (trader_type IN ('HUMAN', 'BOT')),
    bot_strategy TEXT CHECK (bot_strategy IS NULL OR bot_strategy IN ('NORMAL', 'HIGH_VOLUME', 'RAPID_CANCEL', 'RANDOM')),
    cash_balance_paise INTEGER NOT NULL DEFAULT 0 CHECK (cash_balance_paise >= 0),
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS stocks (
    stock_id INTEGER PRIMARY KEY AUTOINCREMENT,
    symbol TEXT NOT NULL UNIQUE,
    company_name TEXT NOT NULL,
    exchange TEXT NOT NULL,
    current_price_paise INTEGER NOT NULL CHECK (current_price_paise > 0),
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS orders (
    order_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE RESTRICT,
    stock_id INTEGER NOT NULL REFERENCES stocks(stock_id) ON DELETE RESTRICT,
    side TEXT NOT NULL CHECK (side IN ('BUY', 'SELL')),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    limit_price_paise INTEGER NOT NULL CHECK (limit_price_paise > 0),
    status TEXT NOT NULL CHECK (status IN ('PENDING', 'PARTIALLY_FILLED', 'EXECUTED', 'CANCELLED', 'REJECTED')),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS trades (
    trade_id INTEGER PRIMARY KEY AUTOINCREMENT,
    order_id INTEGER NOT NULL REFERENCES orders(order_id) ON DELETE RESTRICT,
    executed_quantity INTEGER NOT NULL CHECK (executed_quantity > 0),
    executed_price_paise INTEGER NOT NULL CHECK (executed_price_paise > 0),
    executed_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS positions (
    position_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,
    stock_id INTEGER NOT NULL REFERENCES stocks(stock_id) ON DELETE RESTRICT,
    quantity INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    average_buy_price_paise INTEGER NOT NULL DEFAULT 0 CHECK (average_buy_price_paise >= 0),
    UNIQUE(trader_id, stock_id)
);

CREATE TABLE IF NOT EXISTS alerts (
    alert_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,
    order_id INTEGER REFERENCES orders(order_id) ON DELETE SET NULL,
    rule_name TEXT NOT NULL,
    severity TEXT NOT NULL CHECK (severity IN ('LOW', 'MEDIUM', 'HIGH')),
    risk_points INTEGER NOT NULL CHECK (risk_points BETWEEN 0 AND 100),
    description TEXT NOT NULL,
    created_at TEXT NOT NULL,
    review_status TEXT NOT NULL DEFAULT 'OPEN' CHECK (review_status IN ('OPEN', 'REVIEWED', 'CLEARED')),
    first_seen_at TEXT,
    last_seen_at TEXT,
    occurrence_count INTEGER NOT NULL DEFAULT 1,
    latest_order_id INTEGER REFERENCES orders(order_id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS portfolios (
    portfolio_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL UNIQUE REFERENCES traders(trader_id) ON DELETE CASCADE,
    total_value_paise INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS portfolio_snapshots (
    snapshot_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,
    total_value_paise INTEGER NOT NULL CHECK (total_value_paise >= 0),
    cash_balance_paise INTEGER NOT NULL CHECK (cash_balance_paise >= 0),
    holdings_value_paise INTEGER NOT NULL CHECK (holdings_value_paise >= 0),
    snapshot_time TEXT NOT NULL
);
)";

TEST_CASE("Database Initialization and Foreign Key Enforcement") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);

    // Foreign key violation: inserting order for non-existent trader_id = 999 must throw
    auto stmt = db.prepare(
        "INSERT INTO orders (trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at) "
        "VALUES (999, 1, 'BUY', 10, 1000, 'PENDING', '2026-11-05', '2026-11-05');"
    );
    CHECK_THROWS_AS(stmt.execute(), DatabaseException);
}

TEST_CASE("TraderRepository CRUD and Polymorphic Loading") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);
    TraderRepository repo(db);

    // Insert Human Trader
    HumanTrader human(0, "Lakshya Human", 10000000, "2026-11-05");
    int64_t human_id = repo.insert(human);
    CHECK_EQ(human_id, 1);

    // Insert Bot Trader
    BotTrader bot(0, "AlphaBot", "NORMAL", 50000000, "2026-11-05");
    int64_t bot_id = repo.insert(bot);
    CHECK_EQ(bot_id, 2);

    // Find by ID and check polymorphic types
    auto fetched_human = repo.findById(human_id);
    CHECK(fetched_human.has_value());
    CHECK_EQ(fetched_human.value()->getTraderType(), "HUMAN");
    CHECK_EQ(fetched_human.value()->isBot(), false);
    CHECK_EQ(fetched_human.value()->getCashBalancePaise(), 10000000);

    auto fetched_bot = repo.findById(bot_id);
    CHECK(fetched_bot.has_value());
    CHECK_EQ(fetched_bot.value()->getTraderType(), "BOT");
    CHECK_EQ(fetched_bot.value()->isBot(), true);
    CHECK_EQ(fetched_bot.value()->getBotStrategy().value(), "NORMAL");

    // Update cash balance
    bool updated = repo.updateCashBalance(human_id, 15000000);
    CHECK(updated);
    auto re_fetched = repo.findById(human_id);
    CHECK_EQ(re_fetched.value()->getCashBalancePaise(), 15000000);

    // Fetch all traders
    auto all_traders = repo.findAll();
    CHECK_EQ(all_traders.size(), 2);
}

TEST_CASE("StockRepository CRUD and Price Updates") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);
    StockRepository repo(db);

    Stock stock(0, "RELIANCE", "Reliance Industries Ltd", "NSE", 295000, "2026-11-05T09:15:00Z");
    int64_t id = repo.insert(stock);
    CHECK_EQ(id, 1);

    auto by_id = repo.findById(id);
    CHECK(by_id.has_value());
    CHECK_EQ(by_id.value().getSymbol(), "RELIANCE");

    auto by_symbol = repo.findBySymbol("RELIANCE");
    CHECK(by_symbol.has_value());
    CHECK_EQ(by_symbol.value().getId(), id);

    // Update price
    bool updated = repo.updatePrice(id, 305000, "2026-11-05T09:30:00Z");
    CHECK(updated);
    auto updated_stock = repo.findById(id);
    CHECK_EQ(updated_stock.value().getCurrentPricePaise(), 305000);
}

TEST_CASE("OrderRepository and TradeRepository Operations") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);

    int64_t trader_id = trader_repo.insert(HumanTrader(0, "Lakshya", 10000000, "2026-11-05"));
    int64_t stock_id = stock_repo.insert(Stock(0, "INFY", "Infosys Ltd", "NSE", 150000, "2026-11-05"));

    // Insert Order
    Order order(0, trader_id, stock_id, OrderSide::BUY, 20, 150000, OrderStatus::PENDING, "2026-11-05T10:00:00Z", "2026-11-05T10:00:00Z");
    int64_t order_id = order_repo.insert(order);
    CHECK_EQ(order_id, 1);

    auto fetched_order = order_repo.findById(order_id);
    CHECK(fetched_order.has_value());
    CHECK_EQ(fetched_order.value().getStatus(), OrderStatus::PENDING);

    // Update status to EXECUTED
    order_repo.updateStatus(order_id, OrderStatus::EXECUTED, "2026-11-05T10:00:05Z");
    CHECK_EQ(order_repo.findById(order_id).value().getStatus(), OrderStatus::EXECUTED);

    // Insert Trade execution
    Trade trade(0, order_id, 20, 150000, "2026-11-05T10:00:05Z");
    int64_t trade_id = trade_repo.insert(trade);
    CHECK_EQ(trade_id, 1);

    auto trades = trade_repo.findByOrderId(order_id);
    CHECK_EQ(trades.size(), 1);
    CHECK_EQ(trades[0].getExecutedQuantity(), 20);
    CHECK_EQ(trades[0].getTradeValuePaise(), 3000000); // 20 * 150,000 paise
}

TEST_CASE("PositionRepository and PortfolioRepository Upsert") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    PositionRepository pos_repo(db);
    PortfolioRepository port_repo(db);

    int64_t trader_id = trader_repo.insert(HumanTrader(0, "Lakshya", 10000000, "2026-11-05"));
    int64_t stock_id = stock_repo.insert(Stock(0, "TCS", "Tata Consultancy Services", "NSE", 380000, "2026-11-05"));

    // Initial position insert
    Position pos(0, trader_id, stock_id, 10, 380000);
    pos_repo.saveOrUpdate(pos);

    auto fetched_pos = pos_repo.findByTraderAndStock(trader_id, stock_id);
    CHECK(fetched_pos.has_value());
    CHECK_EQ(fetched_pos.value().getQuantity(), 10);
    CHECK_EQ(fetched_pos.value().getAverageBuyPricePaise(), 380000);

    // Upsert position (simulate adding shares)
    Position updated_pos(0, trader_id, stock_id, 25, 375000);
    pos_repo.saveOrUpdate(updated_pos);

    auto re_pos = pos_repo.findByTraderAndStock(trader_id, stock_id);
    CHECK(re_pos.has_value());
    CHECK_EQ(re_pos.value().getQuantity(), 25);
    CHECK_EQ(re_pos.value().getAverageBuyPricePaise(), 375000);

    // Portfolio upsert
    Portfolio portfolio(0, trader_id, 19375000, "2026-11-05");
    port_repo.saveOrUpdate(portfolio);

    auto fetched_port = port_repo.findByTraderId(trader_id);
    CHECK(fetched_port.has_value());
    CHECK_EQ(fetched_port.value().getTotalValuePaise(), 19375000);

    // Portfolio Snapshots Persistence
    int64_t snap1_id = port_repo.saveSnapshot(trader_id, 19375000, 10000000, 9375000, "2026-11-05T10:00:00Z");
    int64_t snap2_id = port_repo.saveSnapshot(trader_id, 19500000, 9500000, 10000000, "2026-11-05T11:00:00Z");
    CHECK(snap1_id > 0);
    CHECK(snap2_id > snap1_id);

    CHECK_EQ(port_repo.getSnapshotCount(trader_id), 2);

    auto snapshots = port_repo.getSnapshotsByTraderId(trader_id);
    CHECK_EQ(snapshots.size(), 2);
    CHECK_EQ(snapshots[0].getTotalValuePaise(), 19375000);
    CHECK_EQ(snapshots[0].getCashBalancePaise(), 10000000);
    CHECK_EQ(snapshots[0].getHoldingsValuePaise(), 9375000);
    CHECK_EQ(snapshots[0].getSnapshotTime(), "2026-11-05T10:00:00Z");
    CHECK_EQ(snapshots[1].getTotalValuePaise(), 19500000);
}

TEST_CASE("AlertRepository CRUD and Status Review") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);

    TraderRepository trader_repo(db);
    AlertRepository alert_repo(db);

    int64_t trader_id = trader_repo.insert(BotTrader(0, "ShadowBot", "HIGH_VOLUME", 10000000, "2026-11-05"));

    Alert alert(0, trader_id, std::nullopt, "HIGH_VOLUME", AlertSeverity::HIGH, 75,
                "Sudden 10x order spike detected", "2026-11-05T10:30:00Z");
    int64_t alert_id = alert_repo.insert(alert);
    CHECK_EQ(alert_id, 1);

    auto fetched_alert = alert_repo.findById(alert_id);
    CHECK(fetched_alert.has_value());
    CHECK_EQ(fetched_alert.value().getReviewStatus(), AlertReviewStatus::OPEN);
    CHECK_EQ(fetched_alert.value().getRiskPoints(), 75);

    // Review status transition
    bool updated = alert_repo.updateReviewStatus(alert_id, AlertReviewStatus::REVIEWED);
    CHECK(updated);
    CHECK_EQ(alert_repo.findById(alert_id).value().getReviewStatus(), AlertReviewStatus::REVIEWED);
}

TEST_CASE("Database Transactions and RAII Rollback") {
    DatabaseManager db(":memory:");
    db.executeScript(TEST_SCHEMA);
    TraderRepository trader_repo(db);

    // Test committed transaction
    {
        auto tx = db.createTransaction();
        trader_repo.insert(HumanTrader(0, "Committed Trader", 100000, "2026-11-05"));
        tx.commit();
    }
    CHECK_EQ(trader_repo.findAll().size(), 1);

    // Test uncommitted transaction (auto-rollback by RAII TransactionGuard destructor)
    {
        auto tx = db.createTransaction();
        trader_repo.insert(HumanTrader(0, "Uncommitted Trader", 200000, "2026-11-05"));
        // TransactionGuard destructs here without commit() -> rollback
    }
    CHECK_EQ(trader_repo.findAll().size(), 1); // Row count remains 1!
}

TEST_CASE("Schema and Seed File Loading Verification") {
    DatabaseManager db(":memory:");
    CHECK_NOTHROW(db.initializeSchemaFromFile("database/schema.sql"));
    CHECK_NOTHROW(db.loadSeedDataFromFile("database/seed.sql"));

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    PositionRepository pos_repo(db);
    AlertRepository alert_repo(db);
    PortfolioRepository port_repo(db);

    auto traders = trader_repo.findAll();
    CHECK_EQ(traders.size(), 5);

    auto stocks = stock_repo.findAll();
    CHECK_EQ(stocks.size(), 6);

    auto orders = order_repo.findAll();
    CHECK_EQ(orders.size(), 5);

    auto trades = trade_repo.findAll();
    CHECK_EQ(trades.size(), 4);

    auto alerts = alert_repo.findAll();
    CHECK_EQ(alerts.size(), 1);
    CHECK_EQ(alerts[0].getRuleName(), "REPEATED_CANCELLATION");

    auto human = trader_repo.findById(1);
    CHECK(human.has_value());
    CHECK_EQ(human.value()->getName(), "Lakshya (Human Trader)");
    CHECK_EQ(human.value()->getTraderType(), "HUMAN");
    CHECK_EQ(human.value()->getCashBalancePaise(), 10000000);

    auto human_positions = pos_repo.findByTraderId(1);
    CHECK_EQ(human_positions.size(), 3);

    auto reliance = stock_repo.findBySymbol("RELIANCE");
    CHECK(reliance.has_value());
    CHECK_EQ(reliance.value().getCurrentPricePaise(), 295000);
}
