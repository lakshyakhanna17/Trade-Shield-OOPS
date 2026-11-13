#include "TradingEngine.hpp"
#include "detection/FraudEngine.hpp"
#include "detection/TradingEvent.hpp"
#include "utils/TimeUtils.hpp"
#include <limits>
#include <cmath>
#include <iostream>

namespace tradeshield {

TradingEngine::TradingEngine(DatabaseManager& db,
                             TraderRepository& trader_repo,
                             StockRepository& stock_repo,
                             OrderRepository& order_repo,
                             TradeRepository& trade_repo,
                             PositionRepository& position_repo,
                             PortfolioRepository& portfolio_repo)
    : db_(db),
      trader_repo_(trader_repo),
      stock_repo_(stock_repo),
      order_repo_(order_repo),
      trade_repo_(trade_repo),
      position_repo_(position_repo),
      portfolio_repo_(portfolio_repo),
      validator_(trader_repo, stock_repo, position_repo) {}

OrderResult TradingEngine::placeBuyOrder(int64_t trader_id, int64_t stock_id, int64_t quantity, int64_t limit_price_paise) {
    return placeOrder(trader_id, stock_id, OrderSide::BUY, quantity, limit_price_paise);
}

OrderResult TradingEngine::placeSellOrder(int64_t trader_id, int64_t stock_id, int64_t quantity, int64_t limit_price_paise) {
    return placeOrder(trader_id, stock_id, OrderSide::SELL, quantity, limit_price_paise);
}

OrderResult TradingEngine::placeOrder(int64_t trader_id, int64_t stock_id, OrderSide side, int64_t quantity, int64_t limit_price_paise) {
    // 1. Business validation
    auto val_res = validator_.validateNewOrder(trader_id, stock_id, side, quantity, limit_price_paise);
    if (!val_res.is_valid) {
        return {false, val_res.error_message, std::nullopt, std::nullopt};
    }

    auto stock_opt = stock_repo_.findById(stock_id);
    if (!stock_opt.has_value()) {
        return {false, "Stock not found: #" + std::to_string(stock_id), std::nullopt, std::nullopt};
    }

    int64_t current_market_price = stock_opt->getCurrentPricePaise();
    std::string now = TimeUtils::currentIsoTimestamp();

    // 2. Simulated Exchange Matching Logic
    if (side == OrderSide::BUY) {
        if (limit_price_paise >= current_market_price) {
            // Marketable BUY: Execute immediately at current simulated market price
            int64_t execution_price = current_market_price;
            if (execution_price > std::numeric_limits<int64_t>::max() / quantity) {
                return {false, "Execution nominal value causes integer overflow", std::nullopt, std::nullopt};
            }
            int64_t total_execution_cost = quantity * execution_price;

            // Atomic ledger transaction
            auto tx = db_.createTransaction();

            auto trader_opt = trader_repo_.findById(trader_id);
            if (!trader_opt.has_value() || trader_opt.value()->getCashBalancePaise() < total_execution_cost) {
                return {false, "Insufficient cash for immediate trade execution", std::nullopt, std::nullopt};
            }

            // Debit buyer's cash balance
            trader_opt.value()->debitCash(total_execution_cost);
            trader_repo_.updateCashBalance(trader_id, trader_opt.value()->getCashBalancePaise());

            // Update buyer's stock position and moving average buy price
            auto pos_opt = position_repo_.findByTraderAndStock(trader_id, stock_id);
            if (pos_opt.has_value()) {
                pos_opt->addShares(quantity, execution_price);
                position_repo_.saveOrUpdate(*pos_opt);
            } else {
                Position new_pos(0, trader_id, stock_id, quantity, execution_price);
                position_repo_.saveOrUpdate(new_pos);
            }

            // Insert executed Order
            Order order_to_save(0, trader_id, stock_id, side, quantity, limit_price_paise,
                                OrderStatus::EXECUTED, now, now);
            int64_t order_id = order_repo_.insert(order_to_save);
            Order executed_order(order_id, trader_id, stock_id, side, quantity, limit_price_paise,
                                 OrderStatus::EXECUTED, now, now);

            // Insert Trade execution record
            Trade trade_to_save(0, order_id, quantity, execution_price, now);
            int64_t trade_id = trade_repo_.insert(trade_to_save);
            Trade executed_trade(trade_id, order_id, quantity, execution_price, now);

            // Update cached portfolio valuation
            recalculatePortfolioCache(trader_id, now);

            tx.commit();
            emitTradingEvent({TradingEventType::ORDER_EXECUTED, trader_id, executed_order.getId(), stock_id, side, quantity, execution_price, now});
            return {true, "Buy order executed immediately at " + std::to_string(execution_price) + " paise",
                    executed_order, executed_trade};
        } else {
            // Non-marketable BUY: Limit price is below current market price; order remains PENDING
            auto tx = db_.createTransaction();

            Order order_to_save(0, trader_id, stock_id, side, quantity, limit_price_paise,
                                OrderStatus::PENDING, now, now);
            int64_t order_id = order_repo_.insert(order_to_save);
            Order pending_order(order_id, trader_id, stock_id, side, quantity, limit_price_paise,
                                OrderStatus::PENDING, now, now);

            tx.commit();
            emitTradingEvent({TradingEventType::ORDER_SUBMITTED, trader_id, pending_order.getId(), stock_id, side, quantity, limit_price_paise, now});
            return {true, "Buy order placed and pending execution (limit price below market)",
                    pending_order, std::nullopt};
        }
    } else { // OrderSide::SELL
        if (limit_price_paise <= current_market_price) {
            // Marketable SELL: Execute immediately at current simulated market price
            int64_t execution_price = current_market_price;
            if (execution_price > std::numeric_limits<int64_t>::max() / quantity) {
                return {false, "Execution nominal value causes integer overflow", std::nullopt, std::nullopt};
            }
            int64_t total_revenue = quantity * execution_price;

            // Atomic ledger transaction
            auto tx = db_.createTransaction();

            auto pos_opt = position_repo_.findByTraderAndStock(trader_id, stock_id);
            if (!pos_opt.has_value() || pos_opt->getQuantity() < quantity) {
                return {false, "Insufficient stock holdings for immediate execution", std::nullopt, std::nullopt};
            }

            // Deduct shares from seller's position
            pos_opt->deductShares(quantity);
            position_repo_.saveOrUpdate(*pos_opt);

            // Credit seller's cash balance
            auto trader_opt = trader_repo_.findById(trader_id);
            trader_opt.value()->creditCash(total_revenue);
            trader_repo_.updateCashBalance(trader_id, trader_opt.value()->getCashBalancePaise());

            // Insert executed Order
            Order order_to_save(0, trader_id, stock_id, side, quantity, limit_price_paise,
                                OrderStatus::EXECUTED, now, now);
            int64_t order_id = order_repo_.insert(order_to_save);
            Order executed_order(order_id, trader_id, stock_id, side, quantity, limit_price_paise,
                                 OrderStatus::EXECUTED, now, now);

            // Insert Trade execution record
            Trade trade_to_save(0, order_id, quantity, execution_price, now);
            int64_t trade_id = trade_repo_.insert(trade_to_save);
            Trade executed_trade(trade_id, order_id, quantity, execution_price, now);

            // Update cached portfolio valuation
            recalculatePortfolioCache(trader_id, now);

            tx.commit();
            emitTradingEvent({TradingEventType::ORDER_EXECUTED, trader_id, executed_order.getId(), stock_id, side, quantity, execution_price, now});
            return {true, "Sell order executed immediately at " + std::to_string(execution_price) + " paise",
                    executed_order, executed_trade};
        } else {
            // Non-marketable SELL: Limit price is above current market price; order remains PENDING
            auto tx = db_.createTransaction();

            Order order_to_save(0, trader_id, stock_id, side, quantity, limit_price_paise,
                                OrderStatus::PENDING, now, now);
            int64_t order_id = order_repo_.insert(order_to_save);
            Order pending_order(order_id, trader_id, stock_id, side, quantity, limit_price_paise,
                                OrderStatus::PENDING, now, now);

            tx.commit();
            emitTradingEvent({TradingEventType::ORDER_SUBMITTED, trader_id, pending_order.getId(), stock_id, side, quantity, limit_price_paise, now});
            return {true, "Sell order placed and pending execution (limit price above market)",
                    pending_order, std::nullopt};
        }
    }
}

OrderResult TradingEngine::cancelOrder(int64_t order_id) {
    auto order_opt = order_repo_.findById(order_id);
    if (!order_opt.has_value()) {
        return {false, "Order not found: #" + std::to_string(order_id), std::nullopt, std::nullopt};
    }

    auto val_res = validator_.validateCancellation(order_id, *order_opt);
    if (!val_res.is_valid) {
        return {false, val_res.error_message, *order_opt, std::nullopt};
    }

    std::string now = TimeUtils::currentIsoTimestamp();
    auto tx = db_.createTransaction();

    order_opt->cancel(now);
    order_repo_.updateStatus(order_id, OrderStatus::CANCELLED, now);

    tx.commit();
    emitTradingEvent({TradingEventType::ORDER_CANCELLED, order_opt->getTraderId(), order_id, order_opt->getStockId(), order_opt->getSide(), order_opt->getQuantity(), order_opt->getLimitPricePaise(), now});
    return {true, "Order #" + std::to_string(order_id) + " cancelled successfully", *order_opt, std::nullopt};
}

std::vector<Trade> TradingEngine::getExecutionHistory(std::optional<int64_t> order_id, int limit) {
    if (order_id.has_value()) {
        return trade_repo_.findByOrderId(order_id.value());
    }
    return trade_repo_.findAll(limit);
}

std::vector<Order> TradingEngine::getTraderOrders(int64_t trader_id, int limit) {
    return order_repo_.findByTraderId(trader_id, limit);
}

std::vector<Order> TradingEngine::getAllOrders(int limit) {
    return order_repo_.findAll(limit);
}

std::optional<Order> TradingEngine::getOrderById(int64_t order_id) {
    return order_repo_.findById(order_id);
}

std::vector<PositionDetail> TradingEngine::getTraderPositions(int64_t trader_id) {
    std::vector<PositionDetail> details;
    auto positions = position_repo_.findByTraderId(trader_id);

    for (const auto& pos : positions) {
        if (pos.getQuantity() <= 0) {
            continue; // Only active holdings
        }
        auto stock_opt = stock_repo_.findById(pos.getStockId());
        if (!stock_opt.has_value()) {
            continue;
        }

        int64_t current_price = stock_opt->getCurrentPricePaise();
        int64_t total_invested = pos.getTotalInvestedPaise();
        int64_t current_val = pos.getCurrentValuePaise(current_price);
        int64_t pnl = pos.getUnrealizedPnlPaise(current_price);
        double pnl_pct = total_invested > 0 ? (static_cast<double>(pnl) / static_cast<double>(total_invested)) * 100.0 : 0.0;

        details.push_back({
            pos.getId(),
            pos.getStockId(),
            stock_opt->getSymbol(),
            stock_opt->getCompanyName(),
            pos.getQuantity(),
            pos.getAverageBuyPricePaise(),
            current_price,
            total_invested,
            current_val,
            pnl,
            pnl_pct
        });
    }

    return details;
}

std::optional<PortfolioSummary> TradingEngine::getTraderPortfolio(int64_t trader_id) {
    auto trader_opt = trader_repo_.findById(trader_id);
    if (!trader_opt.has_value()) {
        return std::nullopt;
    }

    auto positions = getTraderPositions(trader_id);

    int64_t holdings_value_paise = 0;
    int64_t total_invested_paise = 0;

    for (const auto& pos : positions) {
        holdings_value_paise += pos.current_value_paise;
        total_invested_paise += pos.total_invested_paise;
    }

    int64_t cash = trader_opt.value()->getCashBalancePaise();
    int64_t total_value_paise = cash + holdings_value_paise;
    int64_t unrealized_pnl_paise = holdings_value_paise - total_invested_paise;
    double unrealized_pnl_pct = total_invested_paise > 0
        ? (static_cast<double>(unrealized_pnl_paise) / static_cast<double>(total_invested_paise)) * 100.0
        : 0.0;

    return PortfolioSummary{
        trader_id,
        trader_opt.value()->getName(),
        trader_opt.value()->getTraderType(),
        cash,
        holdings_value_paise,
        total_invested_paise,
        total_value_paise,
        unrealized_pnl_paise,
        unrealized_pnl_pct,
        std::move(positions)
    };
}

void TradingEngine::recalculatePortfolioCache(int64_t trader_id, const std::string& timestamp) {
    auto summary = getTraderPortfolio(trader_id);
    if (summary.has_value()) {
        Portfolio p(0, trader_id, summary->total_value_paise, timestamp);
        portfolio_repo_.saveOrUpdate(p);
        portfolio_repo_.saveSnapshot(trader_id, summary->total_value_paise,
                                     summary->cash_balance_paise, summary->holdings_value_paise,
                                     timestamp);
    }
}

int64_t TradingEngine::recordPortfolioSnapshot(int64_t trader_id, const std::string& timestamp) {
    std::string ts = timestamp.empty() ? TimeUtils::currentIsoTimestamp() : timestamp;
    auto summary = getTraderPortfolio(trader_id);
    if (!summary.has_value()) {
        return -1;
    }
    return portfolio_repo_.saveSnapshot(trader_id, summary->total_value_paise,
                                        summary->cash_balance_paise, summary->holdings_value_paise,
                                        ts);
}

std::vector<PortfolioSnapshot> TradingEngine::getPortfolioSnapshots(int64_t trader_id, int limit) {
    return portfolio_repo_.getSnapshotsByTraderId(trader_id, limit);
}

std::vector<Stock> TradingEngine::getAllStocks() {
    return stock_repo_.findAll();
}

std::optional<Stock> TradingEngine::getStockBySymbol(const std::string& symbol) {
    return stock_repo_.findBySymbol(symbol);
}

bool TradingEngine::updateStockPrice(int64_t stock_id, int64_t new_price_paise) {
    if (new_price_paise <= 0) {
        return false;
    }
    std::string now = TimeUtils::currentIsoTimestamp();
    return stock_repo_.updatePrice(stock_id, new_price_paise, now);
}

std::vector<std::unique_ptr<Trader>> TradingEngine::getAllTraders() {
    return trader_repo_.findAll();
}

void TradingEngine::emitTradingEvent(const TradingEvent& event) {
    if (fraud_engine_) {
        try {
            fraud_engine_->processEvent(event);
        } catch (const std::exception& e) {
            std::cerr << "[TradingEngine] Warning: fraud_engine failed to process event: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[TradingEngine] Warning: unknown error in fraud_engine" << std::endl;
        }
    }
}

} // namespace tradeshield
