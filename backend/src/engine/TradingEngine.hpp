#pragma once

#include "OrderValidator.hpp"
#include "database/DatabaseManager.hpp"
#include "database/repositories/TraderRepository.hpp"
#include "database/repositories/StockRepository.hpp"
#include "database/repositories/OrderRepository.hpp"
#include "database/repositories/TradeRepository.hpp"
#include "database/repositories/PositionRepository.hpp"
#include "database/repositories/PortfolioRepository.hpp"
#include <string>
#include <optional>
#include <vector>
#include <memory>

namespace tradeshield {

/**
 * @brief Detailed breakdown of an individual equity holding.
 */
struct PositionDetail {
    int64_t position_id{0};
    int64_t stock_id{0};
    std::string symbol;
    std::string company_name;
    int64_t quantity{0};
    int64_t average_buy_price_paise{0};
    int64_t current_price_paise{0};
    int64_t total_invested_paise{0};
    int64_t current_value_paise{0};
    int64_t unrealized_pnl_paise{0};
    double unrealized_pnl_percentage{0.0};
};

/**
 * @brief Comprehensive valuation report of a trader's financial portfolio.
 */
struct PortfolioSummary {
    int64_t trader_id{0};
    std::string trader_name;
    std::string trader_type;
    int64_t cash_balance_paise{0};
    int64_t holdings_value_paise{0};
    int64_t total_invested_paise{0};
    int64_t total_value_paise{0};
    int64_t unrealized_pnl_paise{0};
    double unrealized_pnl_percentage{0.0};
    std::vector<PositionDetail> positions;
};

/**
 * @brief Result produced by an order operation (placement, execution, or cancellation).
 */
struct OrderResult {
    bool success{false};
    std::string message;
    std::optional<Order> order;
    std::optional<Trade> trade; // Populated only if the order was executed into a trade
};

/**
 * @brief Central business-logic service coordinating order placement, execution matching,
 * atomic balance and position updates, and portfolio tracking.
 */
class TradingEngine {
public:
    TradingEngine(DatabaseManager& db,
                  TraderRepository& trader_repo,
                  StockRepository& stock_repo,
                  OrderRepository& order_repo,
                  TradeRepository& trade_repo,
                  PositionRepository& position_repo,
                  PortfolioRepository& portfolio_repo);

    // Primary order entry points
    OrderResult placeBuyOrder(int64_t trader_id, int64_t stock_id, int64_t quantity, int64_t limit_price_paise);
    OrderResult placeSellOrder(int64_t trader_id, int64_t stock_id, int64_t quantity, int64_t limit_price_paise);
    OrderResult placeOrder(int64_t trader_id, int64_t stock_id, OrderSide side, int64_t quantity, int64_t limit_price_paise);

    // Order cancellation
    OrderResult cancelOrder(int64_t order_id);

    // Data retrieval and portfolio queries
    std::vector<Trade> getExecutionHistory(std::optional<int64_t> order_id = std::nullopt, int limit = 100);
    std::vector<Order> getTraderOrders(int64_t trader_id, int limit = 100);
    std::vector<Order> getAllOrders(int limit = 100);
    std::optional<Order> getOrderById(int64_t order_id);

    std::optional<PortfolioSummary> getTraderPortfolio(int64_t trader_id);
    std::vector<PositionDetail> getTraderPositions(int64_t trader_id);

    // Real recorded snapshot methods for portfolio performance
    int64_t recordPortfolioSnapshot(int64_t trader_id, const std::string& timestamp = "");
    std::vector<PortfolioSnapshot> getPortfolioSnapshots(int64_t trader_id, int limit = 100);

    std::vector<Stock> getAllStocks();
    std::optional<Stock> getStockBySymbol(const std::string& symbol);
    bool updateStockPrice(int64_t stock_id, int64_t new_price_paise);
    std::vector<std::unique_ptr<Trader>> getAllTraders();

    const OrderValidator& getValidator() const noexcept { return validator_; }

private:
    DatabaseManager& db_;
    TraderRepository& trader_repo_;
    StockRepository& stock_repo_;
    OrderRepository& order_repo_;
    TradeRepository& trade_repo_;
    PositionRepository& position_repo_;
    PortfolioRepository& portfolio_repo_;
    OrderValidator validator_;

    void recalculatePortfolioCache(int64_t trader_id, const std::string& timestamp);
    void emitTradingEvent(const struct TradingEvent& event);

    std::shared_ptr<class FraudEngine> fraud_engine_;
public:
    void setFraudEngine(std::shared_ptr<class FraudEngine> engine) { fraud_engine_ = std::move(engine); }
    std::shared_ptr<class FraudEngine> getFraudEngine() const { return fraud_engine_; }
};

} // namespace tradeshield
