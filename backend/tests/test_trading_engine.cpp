#include "TestFramework.hpp"
#include "database/DatabaseManager.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include "database/repositories/PortfolioRepository.hpp"
#include "engine/TradingEngine.hpp"
#include "engine/OrderValidator.hpp"
#include "api/JsonAdapters.hpp"
#include <memory>

using namespace tradeshield;

static const char* ENGINE_TEST_SCHEMA = R"(
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

struct EngineTestFixture {
    DatabaseManager db{":memory:"};
    TraderRepository trader_repo{db};
    StockRepository stock_repo{db};
    OrderRepository order_repo{db};
    TradeRepository trade_repo{db};
    PositionRepository pos_repo{db};
    PortfolioRepository port_repo{db};
    TradingEngine engine{db, trader_repo, stock_repo, order_repo, trade_repo, pos_repo, port_repo};

    int64_t human_id{0};
    int64_t reliance_id{0}; // Rs 2,500.00 (250,000 paise)
    int64_t infy_id{0};     // Rs 1,500.00 (150,000 paise)

    EngineTestFixture() {
        db.executeScript(ENGINE_TEST_SCHEMA);

        // Seed human trader with Rs 100,000.00 cash (10,000,000 paise)
        human_id = trader_repo.insert(HumanTrader(0, "Lakshya Tester", 10000000, "2026-11-10T09:00:00Z"));

        // Seed stocks
        reliance_id = stock_repo.insert(Stock(0, "RELIANCE", "Reliance Industries", "NSE", 250000, "2026-11-10T09:00:00Z"));
        infy_id = stock_repo.insert(Stock(0, "INFY", "Infosys Ltd", "NSE", 150000, "2026-11-10T09:00:00Z"));

        // Seed initial position: 20 shares of INFY @ Rs 1,400.00 (140,000 paise)
        pos_repo.saveOrUpdate(Position(0, human_id, infy_id, 20, 140000));
    }
};

TEST_CASE("OrderValidator Pre-Trade Checks") {
    EngineTestFixture f;
    const auto& validator = f.engine.getValidator();

    // 1. Valid BUY order
    auto res_buy = validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY, 10, 250000);
    CHECK(res_buy.is_valid);

    // 2. Valid SELL order (owns 20 shares of INFY)
    auto res_sell = validator.validateNewOrder(f.human_id, f.infy_id, OrderSide::SELL, 10, 150000);
    CHECK(res_sell.is_valid);

    // 3. Zero or negative quantity
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY, 0, 250000).is_valid);
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY, -5, 250000).is_valid);

    // 4. Zero or negative limit price
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY, 10, 0).is_valid);
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY, 10, -100).is_valid);

    // 5. Invalid trader ID
    CHECK_FALSE(validator.validateNewOrder(999, f.reliance_id, OrderSide::BUY, 10, 250000).is_valid);

    // 6. Invalid stock ID
    CHECK_FALSE(validator.validateNewOrder(f.human_id, 888, OrderSide::BUY, 10, 250000).is_valid);

    // 7. Insufficient cash for BUY (requires 25,000,000 paise, available 10,000,000 paise)
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY, 100, 250000).is_valid);

    // 8. Insufficient shares for SELL (holds 20 shares of INFY, asks to sell 50)
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.infy_id, OrderSide::SELL, 50, 150000).is_valid);

    // 9. Integer overflow protection
    CHECK_FALSE(validator.validateNewOrder(f.human_id, f.reliance_id, OrderSide::BUY,
                                          1000000000LL, 1000000000LL).is_valid);
}

TEST_CASE("TradingEngine Marketable BUY Execution") {
    EngineTestFixture f;

    // Place marketable BUY: limit price Rs 2,550.00 >= market price Rs 2,500.00 (250,000 paise)
    // Buy 10 shares of RELIANCE. Total cost = 10 * 250,000 = 2,500,000 paise (Rs 25,000.00)
    auto result = f.engine.placeBuyOrder(f.human_id, f.reliance_id, 10, 255000);

    CHECK(result.success);
    CHECK(result.order.has_value());
    CHECK(result.trade.has_value());

    // Verify order properties
    CHECK_EQ(result.order->getStatus(), OrderStatus::EXECUTED);
    CHECK_EQ(result.order->getQuantity(), 10);

    // Verify trade execution at market price (Rs 2,500.00, not the higher limit price)
    CHECK_EQ(result.trade->getExecutedQuantity(), 10);
    CHECK_EQ(result.trade->getExecutedPricePaise(), 250000);
    CHECK_EQ(result.trade->getTradeValuePaise(), 2500000);

    // Verify buyer's cash balance debited: 10,000,000 - 2,500,000 = 7,500,000 paise
    auto buyer = f.trader_repo.findById(f.human_id);
    CHECK(buyer.has_value());
    CHECK_EQ(buyer.value()->getCashBalancePaise(), 7500000);

    // Verify new stock position created
    auto pos = f.pos_repo.findByTraderAndStock(f.human_id, f.reliance_id);
    CHECK(pos.has_value());
    CHECK_EQ(pos->getQuantity(), 10);
    CHECK_EQ(pos->getAverageBuyPricePaise(), 250000);

    // Verify portfolio summary
    auto portfolio = f.engine.getTraderPortfolio(f.human_id);
    CHECK(portfolio.has_value());
    CHECK_EQ(portfolio->cash_balance_paise, 7500000);
    // Holdings: 10 RELIANCE (2,500,000) + 20 INFY (3,000,000) = 5,500,000 paise
    CHECK_EQ(portfolio->holdings_value_paise, 5500000);
    CHECK_EQ(portfolio->total_value_paise, 13000000);
}

TEST_CASE("TradingEngine Marketable SELL Execution") {
    EngineTestFixture f;

    // Sells 10 shares of INFY (currently holds 20 shares @ 140,000 paise).
    // Market price of INFY is 150,000 paise. Limit price is 145,000 paise <= market price.
    // Revenue = 10 * 150,000 = 1,500,000 paise (Rs 15,000.00)
    auto result = f.engine.placeSellOrder(f.human_id, f.infy_id, 10, 145000);

    CHECK(result.success);
    CHECK(result.order.has_value());
    CHECK(result.trade.has_value());

    CHECK_EQ(result.order->getStatus(), OrderStatus::EXECUTED);
    CHECK_EQ(result.trade->getExecutedPricePaise(), 150000);

    // Seller's cash credited: 10,000,000 + 1,500,000 = 11,500,000 paise
    auto seller = f.trader_repo.findById(f.human_id);
    CHECK_EQ(seller.value()->getCashBalancePaise(), 11500000);

    // Holdings reduced: 20 - 10 = 10 shares remaining
    auto pos = f.pos_repo.findByTraderAndStock(f.human_id, f.infy_id);
    CHECK(pos.has_value());
    CHECK_EQ(pos->getQuantity(), 10);
    CHECK_EQ(pos->getAverageBuyPricePaise(), 140000); // Average buy price remains 140,000 paise
}

TEST_CASE("TradingEngine Non-Marketable Limit Orders Remain Pending") {
    EngineTestFixture f;

    // BUY order with limit price Rs 2,400.00 < market price Rs 2,500.00
    auto buy_res = f.engine.placeBuyOrder(f.human_id, f.reliance_id, 5, 240000);
    CHECK(buy_res.success);
    CHECK(buy_res.order.has_value());
    CHECK_FALSE(buy_res.trade.has_value()); // No trade generated!
    CHECK_EQ(buy_res.order->getStatus(), OrderStatus::PENDING);

    // Cash is NOT debited for pending order
    auto buyer = f.trader_repo.findById(f.human_id);
    CHECK_EQ(buyer.value()->getCashBalancePaise(), 10000000);

    // SELL order with limit price Rs 1,600.00 > market price Rs 1,500.00
    auto sell_res = f.engine.placeSellOrder(f.human_id, f.infy_id, 5, 160000);
    CHECK(sell_res.success);
    CHECK(sell_res.order.has_value());
    CHECK_FALSE(sell_res.trade.has_value());
    CHECK_EQ(sell_res.order->getStatus(), OrderStatus::PENDING);

    // Stock quantity is NOT deducted for pending order
    auto pos = f.pos_repo.findByTraderAndStock(f.human_id, f.infy_id);
    CHECK_EQ(pos->getQuantity(), 20);
}

TEST_CASE("Order Lifecycle and Cancellation Safeguards") {
    EngineTestFixture f;

    // Place a pending order
    auto order_res = f.engine.placeBuyOrder(f.human_id, f.reliance_id, 2, 200000);
    CHECK(order_res.success);
    int64_t order_id = order_res.order->getId();

    // 1. Cancel pending order successfully
    auto cancel_res = f.engine.cancelOrder(order_id);
    CHECK(cancel_res.success);
    CHECK_EQ(cancel_res.order->getStatus(), OrderStatus::CANCELLED);

    // Verify status in DB
    auto db_order = f.engine.getOrderById(order_id);
    CHECK(db_order.has_value());
    CHECK_EQ(db_order->getStatus(), OrderStatus::CANCELLED);

    // 2. Repeated cancellation attempt must fail
    auto cancel_again = f.engine.cancelOrder(order_id);
    CHECK_FALSE(cancel_again.success);

    // 3. Attempt to cancel an executed order must fail
    auto exec_res = f.engine.placeBuyOrder(f.human_id, f.reliance_id, 2, 250000);
    CHECK(exec_res.success);
    CHECK_EQ(exec_res.order->getStatus(), OrderStatus::EXECUTED);

    auto cancel_exec = f.engine.cancelOrder(exec_res.order->getId());
    CHECK_FALSE(cancel_exec.success);
    CHECK_EQ(f.engine.getOrderById(exec_res.order->getId())->getStatus(), OrderStatus::EXECUTED);
}

TEST_CASE("Transaction Safety and Atomic Rollback") {
    EngineTestFixture f;

    int64_t initial_cash = f.trader_repo.findById(f.human_id).value()->getCashBalancePaise();
    int64_t initial_orders = f.order_repo.findAll().size();
    int64_t initial_trades = f.trade_repo.findAll().size();

    // Verify rollback behavior when a transaction fails mid-flight
    try {
        auto tx = f.db.createTransaction();

        // 1. Modify cash in DB
        f.trader_repo.updateCashBalance(f.human_id, initial_cash - 500000);

        // 2. Insert order
        f.order_repo.insert(Order(0, f.human_id, f.reliance_id, OrderSide::BUY, 10, 250000,
                                 OrderStatus::EXECUTED, "now", "now"));

        // 3. Simulate unexpected operational exception before commit
        throw std::runtime_error("Simulated catastrophic crash before commit!");

        tx.commit();
    } catch (const std::runtime_error&) {
        // Exception caught, tx went out of scope without commit() -> rollback invoked by RAII
    }

    // Verify state 100% rolled back: cash unchanged, orders count unchanged, trades unchanged
    int64_t restored_cash = f.trader_repo.findById(f.human_id).value()->getCashBalancePaise();
    CHECK_EQ(restored_cash, initial_cash);
    CHECK_EQ(f.order_repo.findAll().size(), initial_orders);
    CHECK_EQ(f.trade_repo.findAll().size(), initial_trades);
}

TEST_CASE("Market Data Consistency and Bounds Invariants") {
    EngineTestFixture f;

    // Test 1: Verify all standard seeded stocks maintain low <= current <= high
    std::vector<std::string> symbols = {"RELIANCE", "TCS", "INFY", "HDFCBANK", "TATAMOTORS", "ICICIBANK"};
    for (const auto& sym : symbols) {
        Stock stock(0, sym, sym + " Limited", "NSE", 100000, "now");
        if (sym == "RELIANCE") stock = Stock(1, sym, "Reliance Industries", "NSE", 295000, "now");
        else if (sym == "TCS") stock = Stock(2, sym, "Tata Consultancy Services", "NSE", 385050, "now");
        else if (sym == "INFY") stock = Stock(3, sym, "Infosys Limited", "NSE", 152000, "now");
        else if (sym == "HDFCBANK") stock = Stock(4, sym, "HDFC Bank Limited", "NSE", 168000, "now");
        else if (sym == "TATAMOTORS") stock = Stock(5, sym, "Tata Motors Limited", "NSE", 94500, "now");
        else if (sym == "ICICIBANK") stock = Stock(6, sym, "ICICI Bank Limited", "NSE", 112000, "now");

        auto md = computeStockMarketData(stock);

        // Core mathematical consistency invariants
        CHECK(md.low_24h_paise <= stock.getCurrentPricePaise());
        CHECK(stock.getCurrentPricePaise() <= md.high_24h_paise);
        CHECK(md.volume > 0);

        // % change calculation against base_open
        double expected_change = (static_cast<double>(stock.getCurrentPricePaise() - md.open_price_paise) /
                                  static_cast<double>(md.open_price_paise)) * 100.0;
        expected_change = std::round(expected_change * 100.0) / 100.0;
        CHECK(std::abs(md.change_24h - expected_change) < 0.01);
    }

    // Test 2: Verify trade aggregation adjusts high, low, and volume correctly
    Stock reliance(1, "RELIANCE", "Reliance Industries", "NSE", 295000, "now");
    int64_t trade_vol = 50000;
    int64_t max_trade_price = 302000;
    int64_t min_trade_price = 289000;
    auto md_with_trades = computeStockMarketData(reliance, trade_vol, max_trade_price, min_trade_price);

    CHECK(md_with_trades.low_24h_paise <= min_trade_price);
    CHECK(md_with_trades.high_24h_paise >= max_trade_price);
    CHECK(md_with_trades.low_24h_paise <= reliance.getCurrentPricePaise());
    CHECK(reliance.getCurrentPricePaise() <= md_with_trades.high_24h_paise);
    CHECK(md_with_trades.volume >= trade_vol);

    // Test 3: Verify TradingEngine updateStockPrice
    CHECK(f.engine.updateStockPrice(f.reliance_id, 298000));
    auto updated_stock = f.stock_repo.findById(f.reliance_id);
    CHECK(updated_stock.has_value());
    CHECK_EQ(updated_stock->getCurrentPricePaise(), 298000);

    // Verify consistency for updated stock
    auto md_updated = computeStockMarketData(*updated_stock);
    CHECK(md_updated.low_24h_paise <= updated_stock->getCurrentPricePaise());
    CHECK(updated_stock->getCurrentPricePaise() <= md_updated.high_24h_paise);

    // Invalid stock price updates should fail
    CHECK_FALSE(f.engine.updateStockPrice(f.reliance_id, 0));
    CHECK_FALSE(f.engine.updateStockPrice(f.reliance_id, -500));
    CHECK_FALSE(f.engine.updateStockPrice(99999, 100000));
}

TEST_CASE("TradingEngine Persists Real Portfolio Snapshots on Trade Execution") {
    EngineTestFixture f;

    // Check baseline snapshots
    auto initial_snaps = f.engine.getPortfolioSnapshots(f.human_id);
    size_t start_count = initial_snaps.size();

    // Place a marketable BUY order that executes
    auto res = f.engine.placeBuyOrder(f.human_id, f.reliance_id, 10, 250000);
    CHECK(res.success);
    CHECK_EQ(res.order->getStatus(), OrderStatus::EXECUTED);

    // Verify snapshot was automatically recorded upon trade execution
    auto updated_snaps = f.engine.getPortfolioSnapshots(f.human_id);
    CHECK_EQ(updated_snaps.size(), start_count + 1);
    CHECK(updated_snaps.back().getTotalValuePaise() > 0);
    CHECK(updated_snaps.back().getHoldingsValuePaise() > 0);
}
