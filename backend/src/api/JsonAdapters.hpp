#pragma once

#include "engine/TradingEngine.hpp"
#include "models/Alert.hpp"
#include "models/PortfolioSnapshot.hpp"
#include "database/repositories/AlertRepository.hpp"
#include "detection/RiskScorer.hpp"
#include "simulation/SimulationManager.hpp"
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>

namespace tradeshield {

using json = nlohmann::json;

inline double paiseToInr(int64_t paise) {
    return static_cast<double>(paise) / 100.0;
}

inline json orderToJson(const Order& o) {
    return json{
        {"order_id", o.getId()},
        {"trader_id", o.getTraderId()},
        {"stock_id", o.getStockId()},
        {"side", orderSideToString(o.getSide())},
        {"quantity", o.getQuantity()},
        {"limit_price_paise", o.getLimitPricePaise()},
        {"limit_price_inr", paiseToInr(o.getLimitPricePaise())},
        {"status", orderStatusToString(o.getStatus())},
        {"created_at", o.getCreatedAt()},
        {"updated_at", o.getUpdatedAt()}
    };
}

inline json tradeToJson(const Trade& t) {
    return json{
        {"trade_id", t.getId()},
        {"order_id", t.getOrderId()},
        {"executed_quantity", t.getExecutedQuantity()},
        {"executed_price_paise", t.getExecutedPricePaise()},
        {"executed_price_inr", paiseToInr(t.getExecutedPricePaise())},
        {"total_value_paise", t.getTradeValuePaise()},
        {"total_value_inr", paiseToInr(t.getTradeValuePaise())},
        {"executed_at", t.getExecutedAt()}
    };
}

inline json positionDetailToJson(const PositionDetail& p) {
    return json{
        {"position_id", p.position_id},
        {"stock_id", p.stock_id},
        {"symbol", p.symbol},
        {"company_name", p.company_name},
        {"quantity", p.quantity},
        {"average_buy_price_paise", p.average_buy_price_paise},
        {"average_buy_price_inr", paiseToInr(p.average_buy_price_paise)},
        {"current_price_paise", p.current_price_paise},
        {"current_price_inr", paiseToInr(p.current_price_paise)},
        {"total_invested_paise", p.total_invested_paise},
        {"total_invested_inr", paiseToInr(p.total_invested_paise)},
        {"current_value_paise", p.current_value_paise},
        {"current_value_inr", paiseToInr(p.current_value_paise)},
        {"unrealized_pnl_paise", p.unrealized_pnl_paise},
        {"unrealized_pnl_inr", paiseToInr(p.unrealized_pnl_paise)},
        {"unrealized_pnl_percentage", p.unrealized_pnl_percentage}
    };
}

inline std::string formatSnapshotTime(const std::string& iso_time) {
    if (iso_time.size() >= 16) {
        std::string date_part = iso_time.substr(8, 2);
        std::string month_part = iso_time.substr(5, 2);
        std::string time_part = iso_time.substr(11, 5);
        std::string m_str = "Nov";
        if (month_part == "01") m_str = "Jan";
        else if (month_part == "02") m_str = "Feb";
        else if (month_part == "03") m_str = "Mar";
        else if (month_part == "04") m_str = "Apr";
        else if (month_part == "05") m_str = "May";
        else if (month_part == "06") m_str = "Jun";
        else if (month_part == "07") m_str = "Jul";
        else if (month_part == "08") m_str = "Aug";
        else if (month_part == "09") m_str = "Sep";
        else if (month_part == "10") m_str = "Oct";
        else if (month_part == "11") m_str = "Nov";
        else if (month_part == "12") m_str = "Dec";
        return date_part + " " + m_str + " " + time_part;
    }
    return iso_time;
}

inline json portfolioSummaryToJson(const PortfolioSummary& s, const std::vector<PortfolioSnapshot>& snapshots = {}) {
    json positions_json = json::array();
    for (const auto& pos : s.positions) {
        positions_json.push_back(positionDetailToJson(pos));
    }

    json perf_history = json::array();
    for (const auto& snap : snapshots) {
        perf_history.push_back({
            {"snapshot_id", snap.getId()},
            {"date", formatSnapshotTime(snap.getSnapshotTime())},
            {"portfolioValue", paiseToInr(snap.getTotalValuePaise())},
            {"cash", paiseToInr(snap.getCashBalancePaise())},
            {"holdings", paiseToInr(snap.getHoldingsValuePaise())},
            {"timestamp", snap.getSnapshotTime()}
        });
    }

    return json{
        {"trader_id", s.trader_id},
        {"trader_name", s.trader_name},
        {"trader_type", s.trader_type},
        {"cash_balance_paise", s.cash_balance_paise},
        {"cash_balance_inr", paiseToInr(s.cash_balance_paise)},
        {"holdings_value_paise", s.holdings_value_paise},
        {"holdings_value_inr", paiseToInr(s.holdings_value_paise)},
        {"total_invested_paise", s.total_invested_paise},
        {"total_invested_inr", paiseToInr(s.total_invested_paise)},
        {"total_value_paise", s.total_value_paise},
        {"total_value_inr", paiseToInr(s.total_value_paise)},
        {"unrealized_pnl_paise", s.unrealized_pnl_paise},
        {"unrealized_pnl_inr", paiseToInr(s.unrealized_pnl_paise)},
        {"unrealized_pnl_percentage", s.unrealized_pnl_percentage},
        {"is_simulated", false},
        {"data_disclaimer", "Real recorded portfolio snapshots from SQLite"},
        {"snapshot_count", snapshots.size()},
        {"performance_history", perf_history},
        {"positions", positions_json}
    };
}

struct StockMarketData {
    int64_t open_price_paise{0};
    int64_t high_24h_paise{0};
    int64_t low_24h_paise{0};
    double change_24h{0.0};
    int64_t volume{0};
};

inline StockMarketData computeStockMarketData(const Stock& s, int64_t trade_volume = 0, int64_t max_trade_price = 0, int64_t min_trade_price = 0) {
    int64_t current = s.getCurrentPricePaise();
    int64_t base_open = current;
    int64_t base_vol = 100000;

    const std::string& sym = s.getSymbol();
    if (sym == "RELIANCE") {
        base_open = 291400; // Rs 2,914.00
        base_vol = 142580;
    } else if (sym == "TCS") {
        base_open = 388200; // Rs 3,882.00
        base_vol = 98400;
    } else if (sym == "INFY") {
        base_open = 151200; // Rs 1,512.00
        base_vol = 185200;
    } else if (sym == "HDFCBANK") {
        base_open = 166200; // Rs 1,662.00
        base_vol = 210400;
    } else if (sym == "TATAMOTORS") {
        base_open = 92300; // Rs 923.00
        base_vol = 320100;
    } else if (sym == "ICICIBANK") {
        base_open = 111500; // Rs 1,115.00
        base_vol = 165000;
    } else {
        base_open = static_cast<int64_t>(current * 0.99);
        base_vol = 50000;
    }

    int64_t high = std::max(current, base_open);
    int64_t low = std::min(current, base_open);

    if (max_trade_price > 0) high = std::max(high, max_trade_price);
    if (min_trade_price > 0) low = std::min(low, min_trade_price);

    int64_t min_spread = std::max<int64_t>(100, current * 6 / 1000); // at least ~0.6%
    if (high - current < min_spread) {
        high = current + min_spread;
    }
    if (current - low < min_spread) {
        low = std::max<int64_t>(100, current - min_spread);
    }

    if (low > current) low = current;
    if (high < current) high = current;

    double change_pct = 0.0;
    if (base_open > 0) {
        change_pct = (static_cast<double>(current - base_open) / static_cast<double>(base_open)) * 100.0;
        change_pct = std::round(change_pct * 100.0) / 100.0;
    }

    StockMarketData data;
    data.open_price_paise = base_open;
    data.high_24h_paise = high;
    data.low_24h_paise = low;
    data.change_24h = change_pct;
    data.volume = base_vol + trade_volume;
    return data;
}

inline json stockToJson(const Stock& s, std::optional<StockMarketData> m = std::nullopt) {
    StockMarketData md = m.has_value() ? *m : computeStockMarketData(s);
    double open_inr = paiseToInr(md.open_price_paise);
    double high_inr = paiseToInr(md.high_24h_paise);
    double low_inr = paiseToInr(md.low_24h_paise);
    double curr_inr = paiseToInr(s.getCurrentPricePaise());

    auto clamp_inr = [&](double v) {
        return std::round(std::max(low_inr, std::min(high_inr, v)) * 100.0) / 100.0;
    };

    json history = json::array({
        {{"time", "09:15"}, {"price", clamp_inr(open_inr)}},
        {{"time", "10:00"}, {"price", clamp_inr(open_inr + (curr_inr - open_inr) * 0.25 + (high_inr - open_inr) * 0.15)}},
        {{"time", "11:00"}, {"price", clamp_inr(open_inr + (curr_inr - open_inr) * 0.40 - (open_inr - low_inr) * 0.20)}},
        {{"time", "12:00"}, {"price", clamp_inr(open_inr + (curr_inr - open_inr) * 0.60 + (high_inr - curr_inr) * 0.25)}},
        {{"time", "13:00"}, {"price", clamp_inr(open_inr + (curr_inr - open_inr) * 0.70 - (curr_inr - low_inr) * 0.15)}},
        {{"time", "14:00"}, {"price", clamp_inr(open_inr + (curr_inr - open_inr) * 0.88 + (high_inr - curr_inr) * 0.10)}},
        {{"time", "15:30"}, {"price", curr_inr}}
    });

    return json{
        {"stock_id", s.getId()},
        {"symbol", s.getSymbol()},
        {"company_name", s.getCompanyName()},
        {"exchange", s.getExchange()},
        {"current_price_paise", s.getCurrentPricePaise()},
        {"current_price_inr", curr_inr},
        {"open_price_paise", md.open_price_paise},
        {"high_24h_paise", md.high_24h_paise},
        {"low_24h_paise", md.low_24h_paise},
        {"change_24h", md.change_24h},
        {"volume", md.volume},
        {"is_simulated", true},
        {"history", history},
        {"updated_at", s.getUpdatedAt()}
    };
}

inline json traderToJson(const Trader& t) {
    json j = {
        {"trader_id", t.getId()},
        {"name", t.getName()},
        {"trader_type", t.getTraderType()},
        {"cash_balance_paise", t.getCashBalancePaise()},
        {"cash_balance_inr", paiseToInr(t.getCashBalancePaise())},
        {"created_at", t.getCreatedAt()},
        {"role_description", t.getRoleDescription()}
    };
    if (t.getBotStrategy().has_value()) {
        j["bot_strategy"] = t.getBotStrategy().value();
    } else {
        j["bot_strategy"] = nullptr;
    }
    return j;
}

inline json alertToJson(const Alert& a) {
    json j = {
        {"alert_id", a.getId()},
        {"trader_id", a.getTraderId()},
        {"rule_name", a.getRuleName()},
        {"severity", alertSeverityToString(a.getSeverity())},
        {"risk_points", a.getRiskPoints()},
        {"description", a.getDescription()},
        {"created_at", a.getCreatedAt()},
        {"review_status", alertReviewStatusToString(a.getReviewStatus())},
        {"first_seen_at", a.getFirstSeenAt()},
        {"last_seen_at", a.getLastSeenAt()},
        {"occurrence_count", a.getOccurrenceCount()}
    };
    if (a.getOrderId().has_value()) {
        j["order_id"] = *a.getOrderId();
    } else {
        j["order_id"] = nullptr;
    }
    if (a.getLatestOrderId().has_value()) {
        j["latest_order_id"] = *a.getLatestOrderId();
    } else {
        j["latest_order_id"] = j["order_id"];
    }
    return j;
}

inline json alertStatsToJson(const AlertStats& s) {
    json j;
    j["total_alerts"] = s.total_alerts;
    j["open_alerts"] = s.open_alerts;
    j["reviewed_alerts"] = s.reviewed_alerts;
    j["cleared_alerts"] = s.cleared_alerts;
    j["high_severity_alerts"] = s.high_severity_alerts;
    j["medium_severity_alerts"] = s.medium_severity_alerts;
    j["low_severity_alerts"] = s.low_severity_alerts;
    j["active_incidents"] = s.active_incidents;
    j["high_critical_alerts"] = s.high_severity_alerts;
    return j;
}

inline json riskProfileToJson(const TraderRiskProfile& p) {
    return json{
        {"trader_id", p.trader_id},
        {"trader_name", p.trader_name},
        {"overall_risk_score", p.overall_risk_score},
        {"risk_tier", p.risk_tier},
        {"total_alerts", p.total_alerts},
        {"low_severity_alerts", p.low_severity_alerts},
        {"medium_severity_alerts", p.medium_severity_alerts},
        {"high_severity_alerts", p.high_severity_alerts},
        {"unreviewed_alerts", p.unreviewed_alerts},
        {"last_alert_time", p.last_alert_time},
        {"triggered_rules", p.triggered_rules}
    };
}

inline json simulationStatusToJson(const SimulationStatus& s) {
    json bots_arr = json::array();
    for (const auto& b : s.active_bots) {
        bots_arr.push_back({
            {"trader_id", b.trader_id},
            {"name", b.name},
            {"strategy", b.strategy}
        });
    }

    return json{
        {"is_running", s.is_running},
        {"tick_interval_ms", s.tick_interval_ms},
        {"total_ticks", s.total_ticks},
        {"total_orders_generated", s.total_orders_generated},
        {"total_executions_generated", s.total_executions_generated},
        {"total_cancellations_generated", s.total_cancellations_generated},
        {"total_rejections", s.total_rejections},
        {"total_system_faults", s.total_system_faults},
        {"total_errors", s.total_errors},
        {"last_tick_time", s.last_tick_time},
        {"active_bots", bots_arr}
    };
}

} // namespace tradeshield
