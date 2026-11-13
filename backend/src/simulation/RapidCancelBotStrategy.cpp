#include "RapidCancelBotStrategy.hpp"
#include <algorithm>

namespace tradeshield {

RapidCancelBotStrategy::RapidCancelBotStrategy(double price_discount_factor)
    : price_discount_factor_(price_discount_factor) {}

BotActionResult RapidCancelBotStrategy::executeTick(TradingEngine& engine, int64_t bot_trader_id) {
    tick_counter_++;

    auto stocks = engine.getAllStocks();
    if (stocks.empty()) {
        return {BotActionType::NONE, false, "No stocks available in catalog"};
    }

    // Step 1: Check if there is an active pending order for this bot.
    // If so, cancel it to accumulate cancellations in the current sliding window.
    auto recent_orders = engine.getTraderOrders(bot_trader_id, 20);
    for (const auto& ord : recent_orders) {
        if (ord.getStatus() == OrderStatus::PENDING) {
            auto cancel_res = engine.cancelOrder(ord.getId());
            return {
                BotActionType::CANCEL_ORDER,
                cancel_res.success,
                cancel_res.message,
                cancel_res.order,
                std::nullopt
            };
        }
    }

    // Step 2: If no orders are pending, submit a non-marketable BUY limit order.
    // Setting limit price below current market price guarantees it remains PENDING.
    const auto& stock = stocks[tick_counter_ % stocks.size()];
    int64_t stock_id = stock.getId();
    int64_t market_price = stock.getCurrentPricePaise();

    int64_t non_marketable_price = static_cast<int64_t>(market_price * price_discount_factor_);
    if (non_marketable_price >= market_price) {
        non_marketable_price = market_price - 100; // Force strictly below market price
    }
    if (non_marketable_price < 100) {
        non_marketable_price = 100;
    }

    // Check affordability before submitting pending limit order
    auto portfolio_opt = engine.getTraderPortfolio(bot_trader_id);
    int64_t cash_available = portfolio_opt.has_value() ? portfolio_opt->cash_balance_paise : 0;
    int64_t max_affordable = (non_marketable_price > 0 && cash_available > 0) ? (cash_available / non_marketable_price) : 0;
    if (max_affordable < 1) {
        return {BotActionType::NONE, true, "Insufficient cash for non-marketable limit order; skipping action"};
    }

    int64_t quantity = std::min<int64_t>(10, max_affordable);
    auto place_res = engine.placeBuyOrder(bot_trader_id, stock_id, quantity, non_marketable_price);

    return {
        BotActionType::BUY_ORDER,
        place_res.success,
        place_res.message,
        place_res.order,
        place_res.trade
    };
}

BotActionResult RapidCancelBotStrategy::executeBurstPair(TradingEngine& engine, int64_t bot_trader_id) {
    auto stocks = engine.getAllStocks();
    if (stocks.empty()) {
        return {BotActionType::NONE, false, "No stocks available in catalog"};
    }

    tick_counter_++;
    const auto& stock = stocks[tick_counter_ % stocks.size()];
    int64_t market_price = stock.getCurrentPricePaise();
    int64_t non_marketable_price = static_cast<int64_t>(market_price * price_discount_factor_);
    if (non_marketable_price >= market_price) {
        non_marketable_price = market_price - 100;
    }

    auto portfolio_opt = engine.getTraderPortfolio(bot_trader_id);
    int64_t cash_available = portfolio_opt.has_value() ? portfolio_opt->cash_balance_paise : 0;
    int64_t max_affordable = (non_marketable_price > 0 && cash_available > 0) ? (cash_available / non_marketable_price) : 0;
    if (max_affordable < 1) {
        return {BotActionType::NONE, true, "Insufficient cash for non-marketable limit burst pair; skipping action"};
    }
    int64_t quantity = std::min<int64_t>(10, max_affordable);

    auto place_res = engine.placeBuyOrder(bot_trader_id, stock.getId(), quantity, non_marketable_price);
    if (!place_res.success || !place_res.order.has_value()) {
        return {BotActionType::BUY_ORDER, false, place_res.message, std::nullopt, std::nullopt};
    }

    int64_t placed_order_id = place_res.order->getId();
    auto cancel_res = engine.cancelOrder(placed_order_id);

    return {
        BotActionType::CANCEL_ORDER,
        cancel_res.success,
        cancel_res.message,
        cancel_res.order,
        std::nullopt
    };
}

} // namespace tradeshield
