#include "TestFramework.hpp"
#include "database/DatabaseManager.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include "database/repositories/PortfolioRepository.hpp"
#include "database/repositories/AlertRepository.hpp"
#include "engine/TradingEngine.hpp"
#include "detection/FraudEngine.hpp"
#include "simulation/IBotStrategy.hpp"
#include "simulation/NormalBotStrategy.hpp"
#include "simulation/SuspiciousBotStrategy.hpp"
#include "simulation/RapidCancelBotStrategy.hpp"
#include "simulation/RandomBotStrategy.hpp"
#include "simulation/SimulationManager.hpp"
#include "api/SimulationController.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

using namespace tradeshield;

static const char* SIM_TEST_SCHEMA = R"(
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
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE RESTRICT,
    stock_id INTEGER NOT NULL REFERENCES stocks(stock_id) ON DELETE RESTRICT,
    quantity INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    average_buy_price_paise INTEGER NOT NULL DEFAULT 0 CHECK (average_buy_price_paise >= 0),
    UNIQUE(trader_id, stock_id)
);

CREATE TABLE IF NOT EXISTS portfolios (
    portfolio_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL UNIQUE REFERENCES traders(trader_id) ON DELETE RESTRICT,
    total_value_paise INTEGER NOT NULL CHECK (total_value_paise >= 0),
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

CREATE TABLE IF NOT EXISTS alerts (
    alert_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE RESTRICT,
    order_id INTEGER REFERENCES orders(order_id) ON DELETE SET NULL,
    rule_name TEXT NOT NULL,
    severity TEXT NOT NULL CHECK (severity IN ('LOW', 'MEDIUM', 'HIGH')),
    risk_points INTEGER NOT NULL CHECK (risk_points >= 0),
    description TEXT NOT NULL,
    created_at TEXT NOT NULL,
    review_status TEXT NOT NULL DEFAULT 'OPEN' CHECK (review_status IN ('OPEN', 'REVIEWED', 'CLEARED')),
    first_seen_at TEXT,
    last_seen_at TEXT,
    occurrence_count INTEGER NOT NULL DEFAULT 1,
    latest_order_id INTEGER REFERENCES orders(order_id) ON DELETE SET NULL
);
)";

struct SimTestHarness {
    DatabaseManager db{":memory:"};
    TraderRepository trader_repo{db};
    StockRepository stock_repo{db};
    OrderRepository order_repo{db};
    TradeRepository trade_repo{db};
    PositionRepository pos_repo{db};
    PortfolioRepository port_repo{db};
    AlertRepository alert_repo{db};

    std::shared_ptr<FraudEngine> fraud_engine;
    std::unique_ptr<TradingEngine> trading_engine;

    int64_t stock_reliance_id{0};
    int64_t stock_infy_id{0};

    int64_t normal_bot_id{0};
    int64_t suspicious_bot_id{0};
    int64_t rapid_cancel_bot_id{0};
    int64_t random_bot_id{0};

    SimTestHarness() {
        db.execute(SIM_TEST_SCHEMA);

        fraud_engine = std::make_shared<FraudEngine>(alert_repo, trader_repo, order_repo, trade_repo);
        trading_engine = std::make_unique<TradingEngine>(
            db, trader_repo, stock_repo, order_repo, trade_repo, pos_repo, port_repo
        );
        trading_engine->setFraudEngine(fraud_engine);

        // Seed stocks
        Stock s1(0, "RELIANCE", "Reliance Industries Ltd", "NSE", 295000, "2026-11-10T12:00:00Z");
        stock_reliance_id = stock_repo.insert(s1);

        Stock s2(0, "INFY", "Infosys Ltd", "NSE", 152000, "2026-11-10T12:00:00Z");
        stock_infy_id = stock_repo.insert(s2);

        // Seed 4 bots with distinct strategies
        BotTrader b1(0, "AlphaBot", "NORMAL", 50000000, "2026-11-10T12:00:00Z");
        normal_bot_id = trader_repo.insert(b1);

        BotTrader b2(0, "ShadowBot", "HIGH_VOLUME", 100000000, "2026-11-10T12:00:00Z");
        suspicious_bot_id = trader_repo.insert(b2);

        BotTrader b3(0, "FlashCancel", "RAPID_CANCEL", 50000000, "2026-11-10T12:00:00Z");
        rapid_cancel_bot_id = trader_repo.insert(b3);

        BotTrader b4(0, "StochasticBot", "RANDOM", 25000000, "2026-11-10T12:00:00Z");
        random_bot_id = trader_repo.insert(b4);

        // Seed initial holdings for ShadowBot (mirrors database/seed.sql)
        Position p1(0, suspicious_bot_id, stock_infy_id, 200, 151000);
        pos_repo.saveOrUpdate(p1);
    }
};

// ============================================================================
// 1. Bot Strategy Unit Tests
// ============================================================================

TEST_CASE("NormalBotStrategy_GeneratesBoundedValidActions") {
    SimTestHarness h;
    NormalBotStrategy strategy;
    CHECK_EQ(strategy.getStrategyName(), "NORMAL");

    // Execute first tick: should place a modest BUY order
    auto res1 = strategy.executeTick(*h.trading_engine, h.normal_bot_id);
    CHECK(res1.success);
    CHECK_EQ(res1.action_type, BotActionType::BUY_ORDER);
    CHECK(res1.order.has_value());
    CHECK(res1.order->getQuantity() <= 15); // Bounded small quantity

    // Verify order was persisted via TradingEngine
    auto orders = h.order_repo.findByTraderId(h.normal_bot_id);
    CHECK_EQ(orders.size(), 1);
}

TEST_CASE("SuspiciousBotStrategy_GeneratesHighVolumeOrderPattern") {
    SimTestHarness h;
    SuspiciousBotStrategy strategy(150); // 150 shares
    CHECK_EQ(strategy.getStrategyName(), "HIGH_VOLUME");

    // Execute tick: places large order >= 100 shares
    auto res = strategy.executeTick(*h.trading_engine, h.suspicious_bot_id);
    CHECK(res.success);
    CHECK_EQ(res.action_type, BotActionType::BUY_ORDER);
    CHECK(res.order.has_value());
    CHECK(res.order->getQuantity() >= 100);

    // HighVolumeRule should have caught this large order via TradingEngine event hook
    auto alerts = h.alert_repo.findByTraderId(h.suspicious_bot_id);
    CHECK_EQ(alerts.size(), 1);
    CHECK_EQ(alerts[0].getRuleName(), "HIGH_VOLUME");
}

TEST_CASE("RapidCancelBotStrategy_PlacesPendingOrderAndCancelsIt") {
    SimTestHarness h;
    RapidCancelBotStrategy strategy(0.85); // 15% discount ensures non-marketable PENDING
    CHECK_EQ(strategy.getStrategyName(), "RAPID_CANCEL");

    // Tick 1: No pending orders exist -> places non-marketable BUY order
    auto res1 = strategy.executeTick(*h.trading_engine, h.rapid_cancel_bot_id);
    CHECK(res1.success);
    CHECK_EQ(res1.action_type, BotActionType::BUY_ORDER);
    CHECK(res1.order.has_value());
    CHECK_EQ(res1.order->getStatus(), OrderStatus::PENDING);

    // Tick 2: Active pending order detected -> cancels it
    auto res2 = strategy.executeTick(*h.trading_engine, h.rapid_cancel_bot_id);
    CHECK(res2.success);
    CHECK_EQ(res2.action_type, BotActionType::CANCEL_ORDER);
    CHECK(res2.order.has_value());
    CHECK_EQ(res2.order->getStatus(), OrderStatus::CANCELLED);
}

TEST_CASE("RandomBotStrategy_DeterministicWithSameSeed") {
    SimTestHarness h1;
    SimTestHarness h2;

    RandomBotStrategy bot1(12345ULL);
    RandomBotStrategy bot2(12345ULL);
    CHECK_EQ(bot1.getStrategyName(), "RANDOM");

    auto res1 = bot1.executeTick(*h1.trading_engine, h1.random_bot_id);
    auto res2 = bot2.executeTick(*h2.trading_engine, h2.random_bot_id);

    CHECK(res1.success);
    CHECK(res2.success);
    CHECK_EQ(res1.action_type, res2.action_type);
    if (res1.order.has_value() && res2.order.has_value()) {
        CHECK_EQ(res1.order->getQuantity(), res2.order->getQuantity());
        CHECK_EQ(res1.order->getStockId(), res2.order->getStockId());
    }
}

// ============================================================================
// 2. Shield Integration & Anomaly Detection from Bot Activity
// ============================================================================

TEST_CASE("ShieldIntegration_SuspiciousBotTriggersHighVolumeAlerts") {
    SimTestHarness h;
    SuspiciousBotStrategy strategy(150);

    // Multiple consecutive large orders should trigger repeated high volume detection
    for (int i = 0; i < 3; ++i) {
        auto res = strategy.executeTick(*h.trading_engine, h.suspicious_bot_id);
        CHECK(res.success);
    }

    auto alerts = h.alert_repo.findByTraderId(h.suspicious_bot_id);
    CHECK(alerts.size() >= 1);

    auto risk = h.fraud_engine->getTraderRisk(h.suspicious_bot_id);
    CHECK(risk.has_value());
    CHECK(risk->overall_risk_score > 0);
}

TEST_CASE("ShieldIntegration_RapidCancelTriggersRepeatedCancellationAlert") {
    SimTestHarness h;
    RapidCancelBotStrategy strategy(0.85);

    // Generate 3 pairs of place-and-cancel
    for (int i = 0; i < 3; ++i) {
        auto burst = strategy.executeBurstPair(*h.trading_engine, h.rapid_cancel_bot_id);
        CHECK(burst.success);
    }

    // Check that RepeatedCancellationRule fired
    auto alerts = h.alert_repo.findByTraderId(h.rapid_cancel_bot_id);
    bool found_cancel_alert = false;
    for (const auto& a : alerts) {
        if (a.getRuleName() == "REPEATED_CANCELLATION") {
            found_cancel_alert = true;
            break;
        }
    }
    CHECK(found_cancel_alert);
}

TEST_CASE("ShieldIntegration_NormalBotMaintainsLowRiskProfile") {
    SimTestHarness h;
    NormalBotStrategy strategy;

    // Normal bot executes 6 ordinary ticks
    for (int i = 0; i < 6; ++i) {
        strategy.executeTick(*h.trading_engine, h.normal_bot_id);
    }

    auto alerts = h.alert_repo.findByTraderId(h.normal_bot_id);
    CHECK_EQ(alerts.size(), 0); // No false-positive alerts

    auto risk = h.fraud_engine->getTraderRisk(h.normal_bot_id);
    CHECK(risk.has_value());
    CHECK_EQ(risk->overall_risk_score, 0);
    CHECK_EQ(risk->risk_tier, "LOW");
}

// ============================================================================
// 3. SimulationManager Lifecycle & Concurrency Tests
// ============================================================================

TEST_CASE("SimulationManager_StartStopLifecycleAndIdempotency") {
    SimTestHarness h;
    SimulationManager sim(*h.trading_engine, h.trader_repo);

    CHECK_FALSE(sim.isRunning());

    // Safe stop when not running
    CHECK(sim.stop());

    // Start with 100ms interval
    CHECK(sim.start(100));
    CHECK(sim.isRunning());

    // Repeated start request must be safely rejected
    CHECK_FALSE(sim.start(100));

    // Allow worker thread to run for 350ms (at least 2 ticks)
    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    // Stop simulation gracefully
    CHECK(sim.stop());
    CHECK_FALSE(sim.isRunning());

    auto status = sim.getStatus();
    CHECK(status.total_ticks >= 2);
    CHECK(status.total_orders_generated >= 2);

    // Safe repeated stop
    CHECK(sim.stop());
    CHECK_FALSE(sim.isRunning());
}

TEST_CASE("SimulationManager_StepTickAndMetricsReset") {
    SimTestHarness h;
    SimulationManager sim(*h.trading_engine, h.trader_repo);

    // Step 2 synchronous ticks
    auto res_tick1 = sim.stepTick();
    CHECK_EQ(res_tick1.size(), 4); // 4 registered bot traders

    auto res_tick2 = sim.stepTick();
    CHECK_EQ(res_tick2.size(), 4);

    auto status1 = sim.getStatus();
    CHECK_EQ(status1.total_ticks, 2);
    CHECK(status1.total_orders_generated > 0);

    // Reset metrics: verifies in-memory counters reset without deleting DB records
    sim.resetMetrics();
    auto status2 = sim.getStatus();
    CHECK_EQ(status2.total_ticks, 0);
    CHECK_EQ(status2.total_orders_generated, 0);

    // Ensure database records are preserved
    auto db_orders = h.order_repo.findAll();
    CHECK(db_orders.size() > 0);
}

TEST_CASE("SimulationManager_BotSelectionFilter") {
    SimTestHarness h;
    SimulationManager sim(*h.trading_engine, h.trader_repo);

    // Start simulation with only AlphaBot (Normal) and ShadowBot (Suspicious)
    std::vector<int64_t> filtered_bots = {h.normal_bot_id, h.suspicious_bot_id};
    CHECK(sim.start(100, filtered_bots));

    auto status = sim.getStatus();
    CHECK_EQ(status.active_bots.size(), 2);
    CHECK_EQ(status.active_bots[0].trader_id, h.normal_bot_id);
    CHECK_EQ(status.active_bots[1].trader_id, h.suspicious_bot_id);

    sim.stop();
}

TEST_CASE("SimulationController_RestEndpoints") {
    using json = nlohmann::json;
    SimTestHarness h;
    SimulationManager sim(*h.trading_engine, h.trader_repo);
    crow::SimpleApp app;
    SimulationController::registerRoutes(app, sim);
    app.validate();

    // 1. GET /api/simulation/status
    {
        crow::request req;
        req.url = "/api/simulation/status";
        req.method = "GET"_method;
        crow::response res;
        app.handle_full(req, res);
        CHECK_EQ(res.code, 200);
        auto j = json::parse(res.body);
        CHECK_EQ(j["is_running"].get<bool>(), false);
        CHECK_EQ(j["total_ticks"].get<uint64_t>(), 0);
    }

    // 2. POST /api/simulation/start
    {
        crow::request req;
        req.url = "/api/simulation/start";
        req.method = "POST"_method;
        req.body = R"({"tick_interval_ms": 200})";
        crow::response res;
        app.handle_full(req, res);
        CHECK_EQ(res.code, 200);
        auto j = json::parse(res.body);
        CHECK_EQ(j["success"].get<bool>(), true);
        CHECK_EQ(j["is_running"].get<bool>(), true);
    }

    // 3. Duplicate POST /api/simulation/start should return 400
    {
        crow::request req;
        req.url = "/api/simulation/start";
        req.method = "POST"_method;
        crow::response res;
        app.handle_full(req, res);
        CHECK_EQ(res.code, 400);
    }

    // 4. POST /api/simulation/stop
    {
        crow::request req;
        req.url = "/api/simulation/stop";
        req.method = "POST"_method;
        crow::response res;
        app.handle_full(req, res);
        CHECK_EQ(res.code, 200);
        auto j = json::parse(res.body);
        CHECK_EQ(j["success"].get<bool>(), true);
        CHECK_EQ(j["is_running"].get<bool>(), false);
    }

    // 5. POST /api/simulation/reset
    {
        crow::request req;
        req.url = "/api/simulation/reset";
        req.method = "POST"_method;
        crow::response res;
        app.handle_full(req, res);
        CHECK_EQ(res.code, 200);
        auto j = json::parse(res.body);
        CHECK_EQ(j["success"].get<bool>(), true);
    }
}

// ============================================================================
// 5. Bot Affordability & Cash Edge Case Verification
// ============================================================================

TEST_CASE("NormalBotStrategy_AffordabilityAndNearZeroCash") {
    SimTestHarness h;
    NormalBotStrategy strategy;

    // Case 1: Bot has sufficient cash (50,000,000 paise = Rs 500,000)
    auto res_suff = strategy.executeTick(*h.trading_engine, h.normal_bot_id);
    CHECK(res_suff.success);
    CHECK_EQ(res_suff.action_type, BotActionType::BUY_ORDER);
    CHECK(res_suff.order.has_value());

    // Case 2: Reduce cash to exactly 2 shares of INFY (152,000 * 2 = 304,000 paise)
    // When buying INFY, it should clamp quantity to 2 rather than trying 5..10
    h.trader_repo.updateCashBalance(h.normal_bot_id, 304000);

    // Run tick - if it picks a stock, quantity must be <= max_affordable
    auto res_clamped = strategy.executeTick(*h.trading_engine, h.normal_bot_id);
    CHECK(res_clamped.success);
    if (res_clamped.action_type == BotActionType::BUY_ORDER && res_clamped.order.has_value()) {
        int64_t cost = res_clamped.order->getQuantity() * res_clamped.order->getLimitPricePaise();
        CHECK(cost <= 304000);
    }

    // Case 3: Set cash to 0 (zero / near-zero cash)
    h.trader_repo.updateCashBalance(h.normal_bot_id, 0);

    // Must gracefully skip action (NONE) instead of submitting an unaffordable BUY order
    auto res_zero = strategy.executeTick(*h.trading_engine, h.normal_bot_id);
    CHECK(res_zero.success);
    // Either NONE or SELL if holding eligible shares
    CHECK(res_zero.action_type == BotActionType::NONE || res_zero.action_type == BotActionType::SELL_ORDER);
    if (res_zero.action_type == BotActionType::NONE) {
        CHECK(!res_zero.order.has_value());
    }
}

TEST_CASE("SuspiciousBotStrategy_AffordabilityClampAndHoldingLiquidation") {
    SimTestHarness h;
    SuspiciousBotStrategy strategy(100);

    // Case 1: Sufficient cash (100,000,000 paise = Rs 1,000,000)
    // RELIANCE @ 295000: 100 shares = 29,500,000 paise.
    auto res_suff = strategy.executeTick(*h.trading_engine, h.suspicious_bot_id);
    CHECK(res_suff.success);

    // Case 2: Clamp case - Cash affords between 1 and 99 shares (e.g. 50 shares of RELIANCE = 14,750,000 paise)
    // Clear existing positions so liquidation does not preempt BUY
    h.db.execute("DELETE FROM positions WHERE trader_id = " + std::to_string(h.suspicious_bot_id));
    h.trader_repo.updateCashBalance(h.suspicious_bot_id, 14750000); // exactly enough for 50 shares of RELIANCE (295,000)

    auto res_clamp = strategy.executeTick(*h.trading_engine, h.suspicious_bot_id);
    CHECK(res_clamp.success);
    if (res_clamp.action_type == BotActionType::BUY_ORDER && res_clamp.order.has_value()) {
        CHECK(res_clamp.order->getQuantity() < 100); // Clamped strictly below 100
        int64_t cost = res_clamp.order->getQuantity() * res_clamp.order->getLimitPricePaise();
        CHECK(cost <= 14750000);
    }

    // Case 3: Zero cash with eligible holdings (seed 200 INFY shares)
    // Bot should sell from eligible holdings to restore liquidity
    h.trader_repo.updateCashBalance(h.suspicious_bot_id, 0);
    Position pos_hold(0, h.suspicious_bot_id, h.stock_infy_id, 200, 151000);
    h.pos_repo.saveOrUpdate(pos_hold);

    auto res_liquidate = strategy.executeTick(*h.trading_engine, h.suspicious_bot_id);
    CHECK(res_liquidate.success);
    CHECK_EQ(res_liquidate.action_type, BotActionType::SELL_ORDER);
    CHECK(res_liquidate.order.has_value());

    // Case 4: Zero cash and ZERO holdings -> must return NONE without failing
    h.db.execute("DELETE FROM positions WHERE trader_id = " + std::to_string(h.suspicious_bot_id));
    h.trader_repo.updateCashBalance(h.suspicious_bot_id, 0);
    auto res_empty = strategy.executeTick(*h.trading_engine, h.suspicious_bot_id);
    CHECK(res_empty.success);
    CHECK_EQ(res_empty.action_type, BotActionType::NONE);
}

TEST_CASE("RandomBotStrategy_AffordabilityClampAndInventorySellFallback") {
    SimTestHarness h;
    RandomBotStrategy strategy(42);

    // Case 1: Sufficient cash
    auto res_suff = strategy.executeTick(*h.trading_engine, h.random_bot_id);
    CHECK(res_suff.success);

    // Case 2: Zero cash with inventory -> falls back to selling inventory
    h.trader_repo.updateCashBalance(h.random_bot_id, 0);
    Position pos(0, h.random_bot_id, h.stock_infy_id, 50, 150000);
    h.pos_repo.saveOrUpdate(pos);

    auto res_sell = strategy.executeTick(*h.trading_engine, h.random_bot_id);
    CHECK(res_sell.success);
    // Must either cancel pending or sell held inventory
    if (res_sell.action_type == BotActionType::SELL_ORDER) {
        CHECK(res_sell.order.has_value());
        CHECK_EQ(res_sell.order->getSide(), OrderSide::SELL);
    }

    // Case 3: Zero cash with NO inventory -> returns NONE
    h.db.execute("DELETE FROM positions WHERE trader_id = " + std::to_string(h.random_bot_id));
    h.trader_repo.updateCashBalance(h.random_bot_id, 0);
    // Clear any pending orders for this bot to prevent cancel path
    h.db.execute("UPDATE orders SET status = 'CANCELLED' WHERE trader_id = " + std::to_string(h.random_bot_id));

    auto res_none = strategy.executeTick(*h.trading_engine, h.random_bot_id);
    CHECK(res_none.success);
    CHECK_EQ(res_none.action_type, BotActionType::NONE);
}

TEST_CASE("Simulation_PreTradeRejections_NoFalseShieldAlerts_NoGhostOrders") {
    SimTestHarness h;

    int initial_order_count = h.order_repo.findAll(100).size();
    int initial_trade_count = h.trade_repo.findAll(100).size();
    int initial_alert_count = h.alert_repo.findByTraderId(h.normal_bot_id).size();

    // Directly attempt an unaffordable order that OrderValidator will reject
    // AlphaBot has 50,000,000 paise. Request 1,000,000 shares @ 295,000 paise = 295,000,000,000 paise
    auto res = h.trading_engine->placeBuyOrder(h.normal_bot_id, h.stock_reliance_id, 1000000, 295000);
    CHECK_FALSE(res.success);
    CHECK_FALSE(res.order.has_value());
    CHECK_FALSE(res.trade.has_value());

    // Verify rejected order did NOT create order row in database
    int current_order_count = h.order_repo.findAll(100).size();
    CHECK_EQ(current_order_count, initial_order_count);

    // Verify no trade was executed
    int current_trade_count = h.trade_repo.findAll(100).size();
    CHECK_EQ(current_trade_count, initial_trade_count);

    // Verify no false Shield alerts were generated
    int current_alert_count = h.alert_repo.findByTraderId(h.normal_bot_id).size();
    CHECK_EQ(current_alert_count, initial_alert_count);
}

TEST_CASE("LongerSimulationRun_NoSpuriousRejectionAccumulation") {
    SimTestHarness h;
    SimulationManager sim(*h.trading_engine, h.trader_repo);

    // Run 50 full ticks across all 4 bots (200 bot decisions)
    for (int t = 0; t < 50; ++t) {
        auto step_results = sim.stepTick();
        CHECK_EQ(step_results.size(), 4);
    }

    auto status = sim.getStatus();
    CHECK_EQ(status.total_ticks, 50);
    CHECK(status.total_orders_generated > 0);
    // Crucial invariant: With affordability checks, pre-trade rejections must be 0!
    CHECK_EQ(status.total_rejections, 0);
    CHECK_EQ(status.total_system_faults, 0);
    CHECK_EQ(status.total_errors, 0);
}
