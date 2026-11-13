#include "SuspiciousBotStrategy.hpp"
#include <algorithm>

namespace tradeshield {

SuspiciousBotStrategy::SuspiciousBotStrategy(int64_t target_quantity)
    : target_quantity_(target_quantity) {}

BotActionResult SuspiciousBotStrategy::executeTick(TradingEngine& engine, int64_t bot_trader_id) {
    tick_counter_++;

    auto stocks = engine.getAllStocks();
    if (stocks.empty()) {
        return {BotActionType::NONE, false, "No stocks available in catalog"};
    }

    const auto& stock = stocks[tick_counter_ % stocks.size()];
    int64_t stock_id = stock.getId();
    int64_t current_price = stock.getCurrentPricePaise();

    auto positions = engine.getTraderPositions(bot_trader_id);
    auto portfolio_opt = engine.getTraderPortfolio(bot_trader_id);
    int64_t cash_available = portfolio_opt.has_value() ? portfolio_opt->cash_balance_paise : 0;

    int64_t target_buy_qty = target_quantity_;
    int64_t estimated_cost = target_buy_qty * current_price;

    // Check if we hold shares of this stock to sell if cash is low or on even ticks
    auto pos_it = std::find_if(positions.begin(), positions.end(), [stock_id](const PositionDetail& p) {
        return p.stock_id == stock_id;
    });

    bool should_sell = (pos_it != positions.end() && pos_it->quantity >= 100 &&
                        (tick_counter_ % 2 == 0 || cash_available < estimated_cost));

    if (should_sell) {
        int64_t sell_qty = std::min<int64_t>(target_quantity_, pos_it->quantity);
        auto sell_res = engine.placeSellOrder(bot_trader_id, stock_id, sell_qty, current_price);
        return {
            BotActionType::SELL_ORDER,
            sell_res.success,
            sell_res.message,
            sell_res.order,
            sell_res.trade
        };
    }

    // If cash is lower than needed, check any other holding with >= 100 shares to liquidate
    if (cash_available < estimated_cost) {
        for (const auto& pos : positions) {
            if (pos.quantity >= 100) {
                auto sell_res = engine.placeSellOrder(bot_trader_id, pos.stock_id, std::min<int64_t>(target_quantity_, pos.quantity), pos.current_price_paise);
                return {
                    BotActionType::SELL_ORDER,
                    sell_res.success,
                    sell_res.message,
                    sell_res.order,
                    sell_res.trade
                };
            }
        }
    }

    // Place high-volume BUY order, strictly clamped to affordable quantity
    int64_t max_affordable = (current_price > 0 && cash_available > 0) ? (cash_available / current_price) : 0;

    if (max_affordable < 1) {
        // Cannot afford 1 share; attempt to sell any eligible remaining inventory to restore liquidity
        for (const auto& pos : positions) {
            if (pos.quantity > 0) {
                int64_t sell_qty = std::min<int64_t>(target_quantity_, pos.quantity);
                auto sell_res = engine.placeSellOrder(bot_trader_id, pos.stock_id, sell_qty, pos.current_price_paise);
                return {
                    BotActionType::SELL_ORDER,
                    sell_res.success,
                    sell_res.message,
                    sell_res.order,
                    sell_res.trade
                };
            }
        }
        return {BotActionType::NONE, true, "Insufficient cash for minimum 1 share; skipping action"};
    }

    // Preserve suspicious high-volume behaviour up to available cash
    int64_t buy_qty = std::min<int64_t>(target_quantity_, max_affordable);
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
