#include "TestFramework.hpp"
#include "models/HumanTrader.hpp"
#include "models/BotTrader.hpp"
#include "models/Stock.hpp"
#include "models/Order.hpp"
#include "models/Trade.hpp"
#include "models/Position.hpp"
#include "models/Portfolio.hpp"
#include "models/Alert.hpp"
#include <memory>

using namespace tradeshield;

TEST_CASE("Trader Construction and Input Validation") {
    // Valid human trader
    CHECK_NOTHROW(HumanTrader(1, "Lakshya", 10000000, "2026-11-05T10:00:00Z"));

    // Empty name should throw
    CHECK_THROWS_AS(HumanTrader(2, "", 10000000, "2026-11-05T10:00:00Z"), ValidationException);

    // Negative initial cash balance should throw
    CHECK_THROWS_AS(HumanTrader(3, "TraderX", -500, "2026-11-05T10:00:00Z"), ValidationException);

    // Valid bot trader
    CHECK_NOTHROW(BotTrader(4, "AlphaBot", "NORMAL", 5000000, "2026-11-05T10:00:00Z"));

    // Invalid bot strategy should throw
    CHECK_THROWS_AS(BotTrader(5, "BadBot", "INVALID_STRATEGY", 5000000, "2026-11-05T10:00:00Z"), ValidationException);
}

TEST_CASE("Trader Financial Encapsulation and Operations") {
    HumanTrader trader(1, "Lakshya", 100000, "2026-11-05T10:00:00Z"); // 100,000 paise = Rs 1,000.00
    CHECK_EQ(trader.getCashBalancePaise(), 100000);

    // Credit cash
    trader.creditCash(50000); // + Rs 500
    CHECK_EQ(trader.getCashBalancePaise(), 150000);

    // Debit cash
    trader.debitCash(30000); // - Rs 300
    CHECK_EQ(trader.getCashBalancePaise(), 120000);

    // Debit exceeding cash balance must throw
    CHECK_THROWS_AS(trader.debitCash(200000), ValidationException);
    CHECK_EQ(trader.getCashBalancePaise(), 120000); // Balance preserved

    // Non-positive credits or debits must throw
    CHECK_THROWS_AS(trader.creditCash(0), ValidationException);
    CHECK_THROWS_AS(trader.creditCash(-100), ValidationException);
    CHECK_THROWS_AS(trader.debitCash(0), ValidationException);
    CHECK_THROWS_AS(trader.debitCash(-100), ValidationException);

    // Setting negative cash balance directly must throw
    CHECK_THROWS_AS(trader.setCashBalancePaise(-1), ValidationException);
}

TEST_CASE("Inheritance and Runtime Polymorphism") {
    std::vector<std::unique_ptr<Trader>> roster;
    roster.push_back(std::make_unique<HumanTrader>(1, "Alice (Human)", 1000000, "2026-11-05"));
    roster.push_back(std::make_unique<BotTrader>(2, "Bot-HighVol", "HIGH_VOLUME", 5000000, "2026-11-05"));
    roster.push_back(std::make_unique<BotTrader>(3, "Bot-Cancel", "RAPID_CANCEL", 2000000, "2026-11-05"));

    // Verify polymorphic virtual method calls
    CHECK_EQ(roster[0]->getTraderType(), "HUMAN");
    CHECK_EQ(roster[0]->isBot(), false);
    CHECK_EQ(roster[0]->getBotStrategy().has_value(), false);
    CHECK_EQ(roster[0]->getRoleDescription(), "Human Manual Trader");

    CHECK_EQ(roster[1]->getTraderType(), "BOT");
    CHECK_EQ(roster[1]->isBot(), true);
    CHECK(roster[1]->getBotStrategy().has_value());
    CHECK_EQ(roster[1]->getBotStrategy().value(), "HIGH_VOLUME");

    CHECK_EQ(roster[2]->getTraderType(), "BOT");
    CHECK_EQ(roster[2]->isBot(), true);
    CHECK_EQ(roster[2]->getBotStrategy().value(), "RAPID_CANCEL");
}

TEST_CASE("Stock Validation and Price Invariants") {
    Stock stock(1, "RELIANCE", "Reliance Industries Ltd", "NSE", 295000, "2026-11-05T09:15:00Z");
    CHECK_EQ(stock.getSymbol(), "RELIANCE");
    CHECK_EQ(stock.getCurrentPricePaise(), 295000);

    // Update price validly
    stock.updatePrice(300000, "2026-11-05T09:30:00Z");
    CHECK_EQ(stock.getCurrentPricePaise(), 300000);
    CHECK_EQ(stock.getUpdatedAt(), "2026-11-05T09:30:00Z");

    // Zero or negative price must throw
    CHECK_THROWS_AS(stock.updatePrice(0, "2026-11-05T09:31:00Z"), ValidationException);
    CHECK_THROWS_AS(stock.updatePrice(-100, "2026-11-05T09:31:00Z"), ValidationException);

    // Construction with empty fields or zero price must throw
    CHECK_THROWS_AS(Stock(2, "", "TCS Ltd", "NSE", 380000, "2026-11-05"), ValidationException);
    CHECK_THROWS_AS(Stock(3, "TCS", "", "NSE", 380000, "2026-11-05"), ValidationException);
    CHECK_THROWS_AS(Stock(4, "TCS", "TCS Ltd", "NSE", 0, "2026-11-05"), ValidationException);
}

TEST_CASE("Order State Transitions and Invariants") {
    Order order(101, 1, 2, OrderSide::BUY, 10, 150000, OrderStatus::PENDING, "2026-11-05T10:00:00Z", "2026-11-05T10:00:00Z");
    CHECK_EQ(order.getTotalValuePaise(), 1500000); // 10 * 150,000 = 1,500,000 paise (Rs 15,000.00)
    CHECK_EQ(order.isTerminal(), false);

    // Execute order
    order.execute("2026-11-05T10:01:00Z");
    CHECK_EQ(order.getStatus(), OrderStatus::EXECUTED);
    CHECK_EQ(order.isTerminal(), true);

    // Executed order cannot be cancelled or rejected
    CHECK_THROWS_AS(order.cancel("2026-11-05T10:02:00Z"), ValidationException);
    CHECK_THROWS_AS(order.reject("2026-11-05T10:02:00Z"), ValidationException);
    CHECK_THROWS_AS(order.execute("2026-11-05T10:02:00Z"), ValidationException);

    // Zero or negative quantity / price in order constructor must throw
    CHECK_THROWS_AS(Order(102, 1, 2, OrderSide::BUY, 0, 10000, OrderStatus::PENDING, "t1", "t1"), ValidationException);
    CHECK_THROWS_AS(Order(103, 1, 2, OrderSide::BUY, 10, 0, OrderStatus::PENDING, "t1", "t1"), ValidationException);
}

TEST_CASE("Position Weighted Average Cost and Unrealized P&L") {
    // Initial empty position: 0 shares @ 0 paise
    Position pos(1, 1, 1, 0, 0);
    CHECK_EQ(pos.getQuantity(), 0);
    CHECK_EQ(pos.getAverageBuyPricePaise(), 0);

    // First purchase: 10 shares @ Rs 100.00 (10,000 paise)
    pos.addShares(10, 10000);
    CHECK_EQ(pos.getQuantity(), 10);
    CHECK_EQ(pos.getAverageBuyPricePaise(), 10000);
    CHECK_EQ(pos.getTotalInvestedPaise(), 100000);

    // Second purchase: 10 shares @ Rs 200.00 (20,000 paise)
    // Weighted average: (10 * 10000 + 10 * 20000) / 20 = 15,000 paise (Rs 150.00)
    pos.addShares(10, 20000);
    CHECK_EQ(pos.getQuantity(), 20);
    CHECK_EQ(pos.getAverageBuyPricePaise(), 15000);
    CHECK_EQ(pos.getTotalInvestedPaise(), 300000);

    // Market value and Unrealized P&L when market price is Rs 180.00 (18,000 paise)
    // Current value = 20 * 18,000 = 360,000 paise
    // Unrealized P&L = 360,000 - 300,000 = +60,000 paise (+Rs 600.00)
    CHECK_EQ(pos.getCurrentValuePaise(18000), 360000);
    CHECK_EQ(pos.getUnrealizedPnlPaise(18000), 60000);

    // Partial sale: sell 5 shares
    pos.deductShares(5);
    CHECK_EQ(pos.getQuantity(), 15);
    CHECK_EQ(pos.getAverageBuyPricePaise(), 15000); // Average buy price remains unchanged on sale!

    // Overselling must throw ValidationException
    CHECK_THROWS_AS(pos.deductShares(20), ValidationException);

    // Selling all remaining shares
    pos.deductShares(15);
    CHECK_EQ(pos.getQuantity(), 0);
    CHECK_EQ(pos.getAverageBuyPricePaise(), 0); // Resets to 0
}

TEST_CASE("Alert Validation and Severity Constraints") {
    // Valid alert
    Alert alert(1, 1, 101, "HIGH_VOLUME", AlertSeverity::HIGH, 85,
                "Order volume exceeds 5x average", "2026-11-05T10:00:00Z");
    CHECK_EQ(alert.getRiskPoints(), 85);
    CHECK_EQ(alert.getSeverity(), AlertSeverity::HIGH);
    CHECK_EQ(alert.getReviewStatus(), AlertReviewStatus::OPEN);

    // Status transition
    alert.setReviewStatus(AlertReviewStatus::REVIEWED);
    CHECK_EQ(alert.getReviewStatus(), AlertReviewStatus::REVIEWED);

    // Risk points out of bounds (<0 or >100) must throw
    CHECK_THROWS_AS(Alert(2, 1, 102, "TEST", AlertSeverity::LOW, -5, "desc", "t"), ValidationException);
    CHECK_THROWS_AS(Alert(3, 1, 102, "TEST", AlertSeverity::LOW, 105, "desc", "t"), ValidationException);
}
