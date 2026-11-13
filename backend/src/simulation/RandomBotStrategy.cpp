#include "RandomBotStrategy.hpp"
#include <algorithm>

namespace tradeshield {

RandomBotStrategy::RandomBotStrategy(uint64_t seed)
    : initial_seed_(seed) {
    setSeed(seed);
}

void RandomBotStrategy::setSeed(uint64_t seed) {
    initial_seed_ = seed;
    if (seed == 0) {
        std::random_device rd;
        rng_.seed(rd());
    } else {
        rng_.seed(seed);
    }
}

BotActionResult RandomBotStrategy::executeTick(TradingEngine& engine, int64_t bot_trader_id) {
    auto stocks = engine.getAllStocks();
    if (stocks.empty()) {
        return {BotActionType::NONE, false, "No stocks available in catalog"};
    }

    std::uniform_int_distribution<size_t> stock_dist(0, stocks.size() - 1);
    const auto& stock = stocks[stock_dist(rng_)];
    int64_t stock_id = stock.getId();
    int64_t current_price = stock.getCurrentPricePaise();

    std::uniform_int_distribution<int> action_dist(0, 99);
    int action_roll = action_dist(rng_);

    // 15% probability: Try cancelling an existing pending order
    if (action_roll < 15) {
        auto recent_orders = engine.getTraderOrders(bot_trader_id, 15);
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

    // 35% probability: Try selling if holding inventory
    if (action_roll >= 15 && action_roll < 50) {
        auto positions = engine.getTraderPositions(bot_trader_id);
        auto pos_it = std::find_if(positions.begin(), positions.end(), [stock_id](const PositionDetail& p) {
            return p.stock_id == stock_id;
        });

        if (pos_it != positions.end() && pos_it->quantity > 0) {
            std::uniform_int_distribution<int64_t> qty_dist(1, std::min<int64_t>(10, pos_it->quantity));
            int64_t sell_qty = qty_dist(rng_);

            // Price variation within +/- 2% using integer paise calculation
            int delta_pct = std::uniform_int_distribution<int>(-2, 2)(rng_);
            int64_t sell_price = current_price + (current_price * delta_pct) / 100;
            if (sell_price < 100) sell_price = 100;

            auto sell_res = engine.placeSellOrder(bot_trader_id, stock_id, sell_qty, sell_price);
            return {
                BotActionType::SELL_ORDER,
                sell_res.success,
                sell_res.message,
                sell_res.order,
                sell_res.trade
            };
        }
    }

    // Default (50% probability, or fallback if sell/cancel was not possible): Place a small/medium BUY order
    // Price variation within +/- 3% using integer paise calculation
    int delta_pct = std::uniform_int_distribution<int>(-3, 3)(rng_);
    int64_t buy_price = current_price + (current_price * delta_pct) / 100;
    if (buy_price < 100) buy_price = 100;

    auto portfolio_opt = engine.getTraderPortfolio(bot_trader_id);
    int64_t cash_available = portfolio_opt.has_value() ? portfolio_opt->cash_balance_paise : 0;
    int64_t max_affordable = (buy_price > 0 && cash_available > 0) ? (cash_available / buy_price) : 0;

    if (max_affordable < 1) {
        // Cannot afford one share; try to choose an eligible SELL action across held inventory
        auto positions = engine.getTraderPositions(bot_trader_id);
        for (const auto& pos : positions) {
            if (pos.quantity > 0) {
                std::uniform_int_distribution<int64_t> qty_dist(1, std::min<int64_t>(10, pos.quantity));
                int64_t sell_qty = qty_dist(rng_);
                int sell_pct = std::uniform_int_distribution<int>(-2, 2)(rng_);
                int64_t s_price = pos.current_price_paise + (pos.current_price_paise * sell_pct) / 100;
                if (s_price < 100) s_price = 100;

                auto sell_res = engine.placeSellOrder(bot_trader_id, pos.stock_id, sell_qty, s_price);
                return {
                    BotActionType::SELL_ORDER,
                    sell_res.success,
                    sell_res.message,
                    sell_res.order,
                    sell_res.trade
                };
            }
        }
        return {BotActionType::NONE, true, "Insufficient cash for minimum 1 share and no inventory to sell; skipping"};
    }

    std::uniform_int_distribution<int64_t> buy_qty_dist(1, 15);
    int64_t requested_qty = buy_qty_dist(rng_);
    int64_t buy_qty = std::min<int64_t>(requested_qty, max_affordable);

    auto buy_res = engine.placeBuyOrder(bot_trader_id, stock_id, buy_qty, buy_price);
    return {
        BotActionType::BUY_ORDER,
        buy_res.success,
        buy_res.message,
        buy_res.order,
        buy_res.trade
    };
}

} // namespace tradeshield
