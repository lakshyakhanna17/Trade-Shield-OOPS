#include "TestFramework.hpp"
#include "database/DatabaseManager.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include "database/repositories/PortfolioRepository.hpp"
#include "database/repositories/AlertRepository.hpp"
#include "detection/HighVolumeRule.hpp"
#include "detection/RepeatedCancellationRule.hpp"
#include "detection/HighFrequencyRule.hpp"
#include "detection/RiskScorer.hpp"
#include "detection/FraudEngine.hpp"
#include "engine/TradingEngine.hpp"
#include "utils/TimeUtils.hpp"
#include <memory>
#include <vector>

using namespace tradeshield;

static const char* FRAUD_TEST_SCHEMA = R"(
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

// ============================================================================
// 1. HighVolumeRule Unit Tests
// ============================================================================

TEST_CASE("HighVolumeRule_NormalOrder_DoesNotTrigger") {
    HighVolumeRule rule(100, 5000000, 3.0); // 100 shares, Rs 50,000 threshold

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_SUBMITTED,
        1, 1, 1, OrderSide::BUY,
        10, 100000, "2026-11-10T12:00:00Z" // 10 shares @ Rs 1000 = Rs 10,000
    };
    ctx.window_seconds = 60;

    auto result = rule.evaluate(ctx);
    CHECK_FALSE(result.triggered);
}

TEST_CASE("HighVolumeRule_IsolatedLargeOrder_TriggersLowSeverity") {
    HighVolumeRule rule(100, 5000000, 3.0);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_SUBMITTED,
        1, 1, 1, OrderSide::BUY,
        200, 30000, "2026-11-10T12:00:00Z" // 200 shares (exceeds 100 qty threshold)
    };
    ctx.window_seconds = 60;

    auto result = rule.evaluate(ctx);
    CHECK(result.triggered);
    CHECK_EQ(result.severity, AlertSeverity::LOW);
    CHECK_EQ(result.risk_points, 20);
}

TEST_CASE("HighVolumeRule_VolumeSpikeOverAverage_TriggersMediumSeverity") {
    HighVolumeRule rule(100, 5000000, 3.0);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.recent_orders = {
        Order(10, 1, 1, OrderSide::BUY, 20, 100000, OrderStatus::EXECUTED, "2026-11-10T11:59:00Z", "2026-11-10T11:59:00Z"),
        Order(11, 1, 1, OrderSide::BUY, 20, 100000, OrderStatus::EXECUTED, "2026-11-10T11:59:30Z", "2026-11-10T11:59:30Z")
    };
    ctx.current_event = {
        TradingEventType::ORDER_SUBMITTED,
        12, 1, 1, OrderSide::BUY,
        120, 100000, "2026-11-10T12:00:00Z" // 120 shares (6x historical average)
    };
    ctx.window_seconds = 60;

    auto result = rule.evaluate(ctx);
    CHECK(result.triggered);
    CHECK_EQ(result.severity, AlertSeverity::MEDIUM);
    CHECK_EQ(result.risk_points, 45);
}

TEST_CASE("HighVolumeRule_RepeatedLargeOrders_TriggersHighSeverity") {
    HighVolumeRule rule(100, 5000000, 3.0);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.recent_orders = {
        Order(10, 1, 1, OrderSide::BUY, 150, 100000, OrderStatus::EXECUTED, "2026-11-10T11:59:40Z", "2026-11-10T11:59:40Z"),
        Order(11, 1, 1, OrderSide::BUY, 200, 100000, OrderStatus::EXECUTED, "2026-11-10T11:59:50Z", "2026-11-10T11:59:50Z")
    };
    ctx.current_event = {
        TradingEventType::ORDER_SUBMITTED,
        12, 1, 1, OrderSide::BUY,
        250, 100000, "2026-11-10T12:00:00Z"
    };
    ctx.window_seconds = 60;

    auto result = rule.evaluate(ctx);
    CHECK(result.triggered);
    CHECK_EQ(result.severity, AlertSeverity::HIGH);
    CHECK_EQ(result.risk_points, 65);
}

// ============================================================================
// 2. RepeatedCancellationRule Unit Tests
// ============================================================================

TEST_CASE("RepeatedCancellationRule_NormalCancellation_DoesNotTrigger") {
    RepeatedCancellationRule rule(60, 3, 0.5);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_CANCELLED,
        1, 1, 1, OrderSide::BUY,
        10, 100000, "2026-11-10T12:00:00Z"
    };
    ctx.recent_orders = {
        Order(1, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:59:50Z", "2026-11-10T12:00:00Z"),
        Order(2, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::EXECUTED, "2026-11-10T11:59:30Z", "2026-11-10T11:59:30Z")
    };

    auto result = rule.evaluate(ctx);
    CHECK_FALSE(result.triggered);
}

TEST_CASE("RepeatedCancellationRule_RapidCancellations_TriggersMediumAndHigh") {
    RepeatedCancellationRule rule(60, 3, 0.5);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_CANCELLED,
        4, 1, 1, OrderSide::BUY,
        10, 100000, "2026-11-10T12:00:00Z"
    };
    ctx.recent_orders = {
        Order(2, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:59:40Z", "2026-11-10T11:59:45Z"),
        Order(3, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:59:50Z", "2026-11-10T11:59:55Z")
    };

    auto result = rule.evaluate(ctx);
    CHECK(result.triggered);
    CHECK_EQ(result.severity, AlertSeverity::MEDIUM);
    CHECK_EQ(result.risk_points, 45);

    // Now test >=5 cancellations triggers HIGH
    ctx.recent_orders.push_back(Order(5, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:59:20Z", "2026-11-10T11:59:25Z"));
    ctx.recent_orders.push_back(Order(6, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:59:30Z", "2026-11-10T11:59:35Z"));
    auto result_high = rule.evaluate(ctx);
    CHECK(result_high.triggered);
    CHECK_EQ(result_high.severity, AlertSeverity::HIGH);
    CHECK_EQ(result_high.risk_points, 70);
}

TEST_CASE("RepeatedCancellationRule_WindowFiltering_IgnoresOldCancellations") {
    RepeatedCancellationRule rule(60, 3, 0.5);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_CANCELLED,
        4, 1, 1, OrderSide::BUY,
        10, 100000, "2026-11-10T12:00:00Z"
    };
    ctx.recent_orders = {
        Order(2, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:50:00Z", "2026-11-10T11:50:05Z"),
        Order(3, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::CANCELLED, "2026-11-10T11:50:10Z", "2026-11-10T11:50:15Z")
    };

    auto result = rule.evaluate(ctx);
    CHECK_FALSE(result.triggered);
}

// ============================================================================
// 3. HighFrequencyRule Unit Tests
// ============================================================================

TEST_CASE("HighFrequencyRule_NormalRate_DoesNotTrigger") {
    HighFrequencyRule rule(10, 4);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_SUBMITTED,
        2, 1, 1, OrderSide::BUY,
        10, 100000, "2026-11-10T12:00:05Z"
    };
    ctx.recent_orders = {
        Order(1, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z")
    };

    auto result = rule.evaluate(ctx);
    CHECK_FALSE(result.triggered);
}

TEST_CASE("HighFrequencyRule_BurstRate_TriggersMediumAndHigh") {
    HighFrequencyRule rule(10, 4);

    DetectionContext ctx;
    ctx.trader_id = 1;
    ctx.current_event = {
        TradingEventType::ORDER_SUBMITTED,
        1, 5, 1, OrderSide::BUY,
        10, 100000, "2026-11-10T12:00:08Z"
    };
    ctx.recent_orders = {
        Order(1, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::PENDING, "2026-11-10T12:00:01Z", "2026-11-10T12:00:01Z"),
        Order(2, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::PENDING, "2026-11-10T12:00:03Z", "2026-11-10T12:00:03Z"),
        Order(3, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::PENDING, "2026-11-10T12:00:06Z", "2026-11-10T12:00:06Z")
    };

    auto result = rule.evaluate(ctx);
    CHECK(result.triggered);
    CHECK_EQ(result.severity, AlertSeverity::MEDIUM);
    CHECK_EQ(result.risk_points, 50);

    for (int i = 6; i <= 9; ++i) {
        ctx.recent_orders.push_back(
            Order(i, 1, 1, OrderSide::BUY, 10, 100000, OrderStatus::PENDING, "2026-11-10T12:00:07Z", "2026-11-10T12:00:07Z")
        );
    }
    auto result_burst = rule.evaluate(ctx);
    CHECK(result_burst.triggered);
    CHECK_EQ(result_burst.severity, AlertSeverity::HIGH);
    CHECK_EQ(result_burst.risk_points, 75);
}

// ============================================================================
// 4. RiskScorer Unit Tests
// ============================================================================

TEST_CASE("RiskScorer_ZeroResults_ReturnsZeroAndLowTier") {
    std::vector<DetectionResult> results;
    int score = RiskScorer::calculateScore(results);
    CHECK_EQ(score, 0);
    CHECK_EQ(RiskScorer::classifySeverity(score), "LOW");
}

TEST_CASE("RiskScorer_CompositeCalculation_CombinesMaxAndOthers") {
    DetectionResult r1;
    r1.triggered = true;
    r1.risk_points = 60;

    DetectionResult r2;
    r2.triggered = true;
    r2.risk_points = 40;

    int score = RiskScorer::calculateScore({r1, r2});
    CHECK_EQ(score, 70);
    CHECK_EQ(RiskScorer::classifySeverity(score), "HIGH");
}

TEST_CASE("RiskScorer_ScoreCapping_ClampsTo100") {
    DetectionResult r1;
    r1.triggered = true;
    r1.risk_points = 80;

    DetectionResult r2;
    r2.triggered = true;
    r2.risk_points = 60;

    DetectionResult r3;
    r3.triggered = true;
    r3.risk_points = 60;

    int score = RiskScorer::calculateScore({r1, r2, r3});
    CHECK_EQ(score, 100);
    CHECK_EQ(RiskScorer::classifySeverity(score), "CRITICAL");
}

TEST_CASE("RiskScorer_SeverityBands_CorrectMapping") {
    CHECK_EQ(RiskScorer::classifySeverity(0), "LOW");
    CHECK_EQ(RiskScorer::classifySeverity(24), "LOW");
    CHECK_EQ(RiskScorer::classifySeverity(25), "MEDIUM");
    CHECK_EQ(RiskScorer::classifySeverity(49), "MEDIUM");
    CHECK_EQ(RiskScorer::classifySeverity(50), "HIGH");
    CHECK_EQ(RiskScorer::classifySeverity(74), "HIGH");
    CHECK_EQ(RiskScorer::classifySeverity(75), "CRITICAL");
    CHECK_EQ(RiskScorer::classifySeverity(100), "CRITICAL");
}

TEST_CASE("RiskScorer_AlertsExclusion_ClearedAlertsIgnored") {
    std::vector<Alert> alerts = {
        Alert(1, 1, 1, "HIGH_VOLUME", AlertSeverity::HIGH, 65, "Active alert", "2026-11-10T12:00:00Z", AlertReviewStatus::OPEN),
        Alert(2, 1, 2, "REPEATED_CANCELLATION", AlertSeverity::HIGH, 70, "Cleared alert", "2026-11-10T12:01:00Z", AlertReviewStatus::CLEARED)
    };

    int score = RiskScorer::calculateScoreFromAlerts(alerts);
    CHECK_EQ(score, 65);
}

// ============================================================================
// 5. FraudEngine Coordinator & Integration Tests
// ============================================================================

TEST_CASE("FraudEngine_PolymorphicDispatch_EvaluatesAllRules") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader human(0, "Alice Trader", 100000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(human);

    Stock stock(0, "TCS", "Tata Consultancy Services", "NSE", 350000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    Order o1(0, trader_id, stock_id, OrderSide::BUY, 10, 10000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z");
    int64_t o1_id = order_repo.insert(o1);

    Order o2(0, trader_id, stock_id, OrderSide::BUY, 500, 10000, OrderStatus::PENDING, "2026-11-10T12:00:05Z", "2026-11-10T12:00:05Z");
    int64_t o2_id = order_repo.insert(o2);

    FraudEngine fraud_engine(alert_repo, trader_repo, order_repo, trade_repo);
    CHECK_EQ(fraud_engine.getRuleCount(), 3);

    TradingEvent ev{TradingEventType::ORDER_SUBMITTED, trader_id, o1_id, stock_id, OrderSide::BUY, 10, 10000, "2026-11-10T12:00:00Z"};
    auto alerts = fraud_engine.processEvent(ev);
    CHECK_EQ(alerts.size(), 0);

    TradingEvent large_ev{TradingEventType::ORDER_SUBMITTED, trader_id, o2_id, stock_id, OrderSide::BUY, 500, 10000, "2026-11-10T12:00:05Z"};
    auto large_alerts = fraud_engine.processEvent(large_ev);
    CHECK_EQ(large_alerts.size(), 1);
    CHECK_EQ(large_alerts[0].getRuleName(), "HIGH_VOLUME");
    CHECK_EQ(large_alerts[0].getSeverity(), AlertSeverity::LOW);
    CHECK_EQ(large_alerts[0].getRiskPoints(), 20);

    auto db_alerts = alert_repo.findByTraderId(trader_id);
    CHECK_EQ(db_alerts.size(), 1);
}

TEST_CASE("FraudEngine_DuplicatePrevention_DoesNotDuplicateAlert") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader human(0, "Bob Trader", 100000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(human);

    Stock stock(0, "INFY", "Infosys Limited", "NSE", 150000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    Order o1(0, trader_id, stock_id, OrderSide::BUY, 500, 10000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z");
    int64_t o1_id = order_repo.insert(o1);

    FraudEngine fraud_engine(alert_repo, trader_repo, order_repo, trade_repo);

    TradingEvent large_ev{TradingEventType::ORDER_SUBMITTED, trader_id, o1_id, stock_id, OrderSide::BUY, 500, 10000, "2026-11-10T12:00:00Z"};
    auto alerts1 = fraud_engine.processEvent(large_ev);
    CHECK_EQ(alerts1.size(), 1);

    auto alerts2 = fraud_engine.processEvent(large_ev);
    CHECK_EQ(alerts2.size(), 0);

    auto all_alerts = alert_repo.findAll();
    CHECK_EQ(all_alerts.size(), 1);
}

TEST_CASE("FraudEngine_IntegrationWithTradingEngine_OrderTriggersAlert") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    PositionRepository pos_repo(db);
    PortfolioRepository port_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader trader(0, "Active Trader", 50000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(trader);

    Stock stock(0, "RELIANCE", "Reliance Industries", "NSE", 250000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    auto fraud_engine = std::make_shared<FraudEngine>(alert_repo, trader_repo, order_repo, trade_repo);

    TradingEngine trading_engine(db, trader_repo, stock_repo, order_repo, trade_repo, pos_repo, port_repo);
    trading_engine.setFraudEngine(fraud_engine);

    auto res = trading_engine.placeBuyOrder(trader_id, stock_id, 150, 250000);
    CHECK(res.success);

    auto alerts = alert_repo.findByTraderId(trader_id);
    CHECK_EQ(alerts.size(), 1);
    CHECK_EQ(alerts[0].getRuleName(), "HIGH_VOLUME");

    auto risk_profile = fraud_engine->getTraderRisk(trader_id);
    CHECK(risk_profile.has_value());
    CHECK_EQ(risk_profile->total_alerts, 1);
    CHECK_EQ(risk_profile->overall_risk_score, 20);
    CHECK_EQ(risk_profile->risk_tier, "LOW");
}

TEST_CASE("AlertRepository_FiltersAndReviewStatus") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    HumanTrader t1(0, "Trader One", 10000000, "2026-11-10T12:00:00Z");
    HumanTrader t2(0, "Trader Two", 10000000, "2026-11-10T12:00:00Z");
    int64_t id1 = trader_repo.insert(t1);
    int64_t id2 = trader_repo.insert(t2);

    AlertRepository alert_repo(db);

    Alert a1(0, id1, std::nullopt, "HIGH_VOLUME", AlertSeverity::LOW, 20, "Test alert 1", "2026-11-10T12:00:00Z", AlertReviewStatus::OPEN);
    Alert a2(0, id1, std::nullopt, "HIGH_FREQUENCY", AlertSeverity::HIGH, 75, "Test alert 2", "2026-11-10T12:01:00Z", AlertReviewStatus::OPEN);
    Alert a3(0, id2, std::nullopt, "HIGH_VOLUME", AlertSeverity::MEDIUM, 45, "Test alert 3", "2026-11-10T12:02:00Z", AlertReviewStatus::OPEN);

    int64_t a1_id = alert_repo.insert(a1);
    alert_repo.insert(a2);
    int64_t a3_id = alert_repo.insert(a3);

    auto t1_alerts = alert_repo.findByFilters(id1, std::nullopt, std::nullopt);
    CHECK_EQ(t1_alerts.size(), 2);

    auto high_alerts = alert_repo.findByFilters(std::nullopt, AlertSeverity::HIGH, std::nullopt);
    CHECK_EQ(high_alerts.size(), 1);

    bool updated = alert_repo.updateReviewStatus(a1_id, AlertReviewStatus::REVIEWED);
    CHECK(updated);
    auto reviewed = alert_repo.findById(a1_id);
    CHECK(reviewed.has_value());
    CHECK_EQ(reviewed->getReviewStatus(), AlertReviewStatus::REVIEWED);

    alert_repo.updateReviewStatus(a3_id, AlertReviewStatus::CLEARED);
    auto cleared_alerts = alert_repo.findByFilters(std::nullopt, std::nullopt, AlertReviewStatus::CLEARED);
    CHECK_EQ(cleared_alerts.size(), 1);
    CHECK_EQ(cleared_alerts[0].getId(), a3_id);
}

TEST_CASE("IncidentAggregation_RepeatedDetectionsAndOccurrenceCount") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader trader(0, "Aggregated Trader", 10000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(trader);

    Stock stock(0, "RELIANCE", "Reliance Industries", "NSE", 250000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    Order o1(0, trader_id, stock_id, OrderSide::BUY, 150, 250000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z");
    int64_t ord1 = order_repo.insert(o1);
    Order o2(0, trader_id, stock_id, OrderSide::BUY, 150, 250000, OrderStatus::PENDING, "2026-11-10T12:00:10Z", "2026-11-10T12:00:10Z");
    int64_t ord2 = order_repo.insert(o2);

    FraudEngine fraud_engine(alert_repo, trader_repo, order_repo, trade_repo);

    // Event 1: First high volume order
    TradingEvent e1{TradingEventType::ORDER_SUBMITTED, trader_id, ord1, stock_id, OrderSide::BUY, 150, 250000, "2026-11-10T12:00:00Z"};
    auto alerts1 = fraud_engine.processEvent(e1);
    CHECK_EQ(alerts1.size(), 1);
    CHECK_EQ(alerts1[0].getOccurrenceCount(), 1);
    CHECK_EQ(alerts1[0].getFirstSeenAt(), "2026-11-10T12:00:00Z");
    CHECK_EQ(alerts1[0].getLastSeenAt(), "2026-11-10T12:00:00Z");

    int64_t incident_id = alerts1[0].getId();
    CHECK_EQ(alert_repo.findAll().size(), 1);

    // Event 2: Second high volume order within episode window (10 seconds later)
    TradingEvent e2{TradingEventType::ORDER_SUBMITTED, trader_id, ord2, stock_id, OrderSide::BUY, 150, 250000, "2026-11-10T12:00:10Z"};
    auto alerts2 = fraud_engine.processEvent(e2);
    CHECK_EQ(alerts2.size(), 1);
    CHECK_EQ(alerts2[0].getId(), incident_id); // Same incident identity!
    CHECK_EQ(alerts2[0].getOccurrenceCount(), 2);
    CHECK_EQ(alerts2[0].getFirstSeenAt(), "2026-11-10T12:00:00Z");
    CHECK_EQ(alerts2[0].getLastSeenAt(), "2026-11-10T12:00:10Z");
    CHECK_EQ(*alerts2[0].getLatestOrderId(), ord2);

    // Total persisted records in DB must STILL be 1 (no spamming!)
    CHECK_EQ(alert_repo.findAll().size(), 1);
}

TEST_CASE("IncidentAggregation_SeverityEscalation") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader trader(0, "Escalating Trader", 10000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(trader);
    Stock stock(0, "RELIANCE", "Reliance Industries", "NSE", 250000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    Order o1(0, trader_id, stock_id, OrderSide::BUY, 100, 250000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z");
    int64_t ord1 = order_repo.insert(o1);
    Order o2(0, trader_id, stock_id, OrderSide::BUY, 200, 250000, OrderStatus::PENDING, "2026-11-10T12:00:15Z", "2026-11-10T12:00:15Z");
    int64_t ord2 = order_repo.insert(o2);

    // Initial alert: MEDIUM severity
    Alert initial(0, trader_id, ord1, "HIGH_VOLUME", AlertSeverity::MEDIUM, 45,
                  "Isolated large order", "2026-11-10T12:00:00Z", AlertReviewStatus::OPEN,
                  "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z", 1, ord1);
    int64_t alert_id = alert_repo.insert(initial);

    // Escalate with new evidence: HIGH severity
    bool updated = alert_repo.updateIncident(alert_id, "2026-11-10T12:00:15Z", 2, ord2,
                                            AlertSeverity::HIGH, 65, "Persistent abnormal volume");
    CHECK(updated);

    auto escalated = alert_repo.findById(alert_id);
    CHECK(escalated.has_value());
    CHECK_EQ(escalated->getSeverity(), AlertSeverity::HIGH);
    CHECK_EQ(escalated->getRiskPoints(), 65);
    CHECK_EQ(escalated->getOccurrenceCount(), 2);
    CHECK_EQ(*escalated->getLatestOrderId(), ord2);
}

TEST_CASE("IncidentLifecycle_NewIncidentAfterClosure") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader trader(0, "Lifecycle Trader", 10000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(trader);
    Stock stock(0, "INFY", "Infosys", "NSE", 150000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    Order o1(0, trader_id, stock_id, OrderSide::BUY, 150, 150000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z");
    int64_t ord1 = order_repo.insert(o1);
    Order o2(0, trader_id, stock_id, OrderSide::BUY, 150, 150000, OrderStatus::PENDING, "2026-11-10T12:01:00Z", "2026-11-10T12:01:00Z");
    int64_t ord2 = order_repo.insert(o2);

    FraudEngine fraud_engine(alert_repo, trader_repo, order_repo, trade_repo);

    // 1. Initial incident
    TradingEvent e1{TradingEventType::ORDER_SUBMITTED, trader_id, ord1, stock_id, OrderSide::BUY, 150, 150000, "2026-11-10T12:00:00Z"};
    auto alerts1 = fraud_engine.processEvent(e1);
    CHECK_EQ(alerts1.size(), 1);
    int64_t incident1_id = alerts1[0].getId();

    // 2. Compliance marks incident as REVIEWED
    alert_repo.updateReviewStatus(incident1_id, AlertReviewStatus::REVIEWED);
    auto closed = alert_repo.findById(incident1_id);
    CHECK_EQ(closed->getReviewStatus(), AlertReviewStatus::REVIEWED);

    // 3. New suspicious event occurs after review
    TradingEvent e2{TradingEventType::ORDER_SUBMITTED, trader_id, ord2, stock_id, OrderSide::BUY, 150, 150000, "2026-11-10T12:01:00Z"};
    auto alerts2 = fraud_engine.processEvent(e2);
    CHECK_EQ(alerts2.size(), 1);
    int64_t incident2_id = alerts2[0].getId();

    // Must NOT reopen incident 1; must create a new distinct incident 2!
    CHECK(incident2_id != incident1_id);
    CHECK_EQ(alerts2[0].getReviewStatus(), AlertReviewStatus::OPEN);
    CHECK_EQ(alerts2[0].getOccurrenceCount(), 1);

    // Historical incident 1 must still remain REVIEWED
    auto original = alert_repo.findById(incident1_id);
    CHECK_EQ(original->getReviewStatus(), AlertReviewStatus::REVIEWED);
    CHECK_EQ(alert_repo.findAll().size(), 2);
}

TEST_CASE("IncidentAggregation_MultipleTradersIndependent") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    StockRepository stock_repo(db);
    OrderRepository order_repo(db);
    TradeRepository trade_repo(db);
    AlertRepository alert_repo(db);

    HumanTrader t1(0, "Trader A", 10000000, "2026-11-10T12:00:00Z");
    HumanTrader t2(0, "Trader B", 10000000, "2026-11-10T12:00:00Z");
    int64_t id1 = trader_repo.insert(t1);
    int64_t id2 = trader_repo.insert(t2);

    Stock stock(0, "TCS", "Tata Consultancy Services", "NSE", 380000, "2026-11-10T12:00:00Z");
    int64_t stock_id = stock_repo.insert(stock);

    Order o1(0, id1, stock_id, OrderSide::BUY, 120, 380000, OrderStatus::PENDING, "2026-11-10T12:00:00Z", "2026-11-10T12:00:00Z");
    int64_t ord1 = order_repo.insert(o1);
    Order o2(0, id2, stock_id, OrderSide::BUY, 120, 380000, OrderStatus::PENDING, "2026-11-10T12:00:05Z", "2026-11-10T12:00:05Z");
    int64_t ord2 = order_repo.insert(o2);

    FraudEngine fraud_engine(alert_repo, trader_repo, order_repo, trade_repo);

    TradingEvent e1{TradingEventType::ORDER_SUBMITTED, id1, ord1, stock_id, OrderSide::BUY, 120, 380000, "2026-11-10T12:00:00Z"};
    TradingEvent e2{TradingEventType::ORDER_SUBMITTED, id2, ord2, stock_id, OrderSide::BUY, 120, 380000, "2026-11-10T12:00:05Z"};

    auto a1 = fraud_engine.processEvent(e1);
    auto a2 = fraud_engine.processEvent(e2);

    CHECK_EQ(a1.size(), 1);
    CHECK_EQ(a2.size(), 1);
    CHECK(a1[0].getId() != a2[0].getId());
    CHECK_EQ(a1[0].getTraderId(), id1);
    CHECK_EQ(a2[0].getTraderId(), id2);
}

TEST_CASE("AlertRepository_StatsAndPagination") {
    DatabaseManager db(":memory:");
    db.execute(FRAUD_TEST_SCHEMA);

    TraderRepository trader_repo(db);
    HumanTrader t(0, "Stats Trader", 10000000, "2026-11-10T12:00:00Z");
    int64_t trader_id = trader_repo.insert(t);

    AlertRepository alert_repo(db);

    // Insert 4 alerts (order_id nullopt to avoid foreign key requirements)
    alert_repo.insert(Alert(0, trader_id, std::nullopt, "HIGH_VOLUME", AlertSeverity::HIGH, 75, "Desc 1", "2026-11-10T12:00:00Z", AlertReviewStatus::OPEN));
    alert_repo.insert(Alert(0, trader_id, std::nullopt, "HIGH_VOLUME", AlertSeverity::MEDIUM, 45, "Desc 2", "2026-11-10T12:01:00Z", AlertReviewStatus::OPEN));
    alert_repo.insert(Alert(0, trader_id, std::nullopt, "HIGH_FREQUENCY", AlertSeverity::LOW, 20, "Desc 3", "2026-11-10T12:02:00Z", AlertReviewStatus::REVIEWED));
    alert_repo.insert(Alert(0, trader_id, std::nullopt, "REPEATED_CANCELLATION", AlertSeverity::HIGH, 70, "Desc 4", "2026-11-10T12:03:00Z", AlertReviewStatus::CLEARED));

    // Verify stats
    auto stats = alert_repo.getStats();
    CHECK_EQ(stats.total_alerts, 4);
    CHECK_EQ(stats.open_alerts, 2);
    CHECK_EQ(stats.reviewed_alerts, 1);
    CHECK_EQ(stats.cleared_alerts, 1);
    CHECK_EQ(stats.high_severity_alerts, 1); // 1 OPEN high severity
    CHECK_EQ(stats.medium_severity_alerts, 1); // 1 OPEN medium severity

    // Verify pagination and counting
    CHECK_EQ(alert_repo.countByFilters(std::nullopt, std::nullopt, std::nullopt), 4);
    auto page1 = alert_repo.findByFilters(std::nullopt, std::nullopt, std::nullopt, 2, 0);
    CHECK_EQ(page1.size(), 2);
    auto page2 = alert_repo.findByFilters(std::nullopt, std::nullopt, std::nullopt, 2, 2);
    CHECK_EQ(page2.size(), 2);
    CHECK(page1[0].getId() != page2[0].getId());
}
