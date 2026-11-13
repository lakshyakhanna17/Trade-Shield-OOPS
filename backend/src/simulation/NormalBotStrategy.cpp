#include "NormalBotStrategy.hpp"
#include <algorithm>

namespace tradeshield {

BotActionResult NormalBotStrategy::executeTick(TradingEngine& engine, int64_t bot_trader_id) {
    tick_counter_++;

    auto stocks = engine.getAllStocks();
    if (stocks.empty()) {
        return {BotActionType::NONE, false, "No stocks available in market catalog"};
    }

    // Every 5th tick, if the bot has an active pending order, occasionally cancel it
    if (tick_counter_ % 5 == 0) {
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
    }

    // Select stock via round-robin index
    const auto& stock = stocks[tick_counter_ % stocks.size()];
    int64_t stock_id = stock.getId();
    int64_t current_price = stock.getCurrentPricePaise();

    // Check if the bot currently holds shares in this stock
    auto positions = engine.getTraderPositions(bot_trader_id);
    auto pos_it = std::find_if(positions.begin(), positions.end(), [stock_id](const PositionDetail& p) {
        return p.stock_id == stock_id;
    });

    // If holding shares and on an odd tick, place a modest SELL order (2 to 5 shares)
    if (pos_it != positions.end() && pos_it->quantity >= 5 && (tick_counter_ % 2 == 1)) {
        int64_t sell_qty = std::min<int64_t>(5, pos_it->quantity / 2);
        if (sell_qty < 1) sell_qty = 1;

        auto sell_res = engine.placeSellOrder(bot_trader_id, stock_id, sell_qty, current_price);
        return {
            BotActionType::SELL_ORDER,
            sell_res.success,
            sell_res.message,
            sell_res.order,
            sell_res.trade
        };
    }

    // Otherwise, place a modest BUY order (5 to 10 shares, well below 100 qty threshold)
    // Check available cash before submitting BUY order
    auto portfolio_opt = engine.getTraderPortfolio(bot_trader_id);
    int64_t cash_available = portfolio_opt.has_value() ? portfolio_opt->cash_balance_paise : 0;

    if (cash_available <= 0 || current_price <= 0) {
        return {BotActionType::NONE, true, "Insufficient cash for minimum 1 share; skipping action"};
    }

    int64_t max_affordable = cash_available / current_price;
    if (max_affordable < 1) {
        return {BotActionType::NONE, true, "Insufficient cash for minimum 1 share; skipping action"};
    }

    int64_t intended_qty = 5 + (tick_counter_ % 6); // 5..10 shares
    int64_t buy_qty = std::min<int64_t>(intended_qty, max_affordable);
    auto buy_res = engine.placeBuyOrder(bot_trader_id, stock_id, buy_qty, current_price);

    return {
        BotActionType::BUY_ORDER,
        buy_res.success,
        buy_res.message,
        buy_res.order,
        buy_res.trade
    };
}

} // namespace tradeshield
