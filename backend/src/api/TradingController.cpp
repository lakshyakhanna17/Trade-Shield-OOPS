#include "TradingController.hpp"
#include "JsonAdapters.hpp"
#include <iostream>

namespace tradeshield {

crow::response TradingController::makeJsonResponse(int code, const std::string& body) {
    crow::response res(code, body);
    res.set_header("Content-Type", "application/json");
    return res;
}

crow::response TradingController::makeErrorResponse(int code, const std::string& error_msg) {
    json j = {
        {"success", false},
        {"error", error_msg}
    };
    return makeJsonResponse(code, j.dump());
}

template<typename AppType>
void TradingController::registerRoutes(AppType& app,
                                       TradingEngine& engine,
                                       AlertRepository& alert_repo,
                                       FraudEngine& fraud_engine) {
    // ----------------------------------------------------
    // POST /api/orders - Place a buy or sell order
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/orders").methods("POST"_method)
    ([&engine](const crow::request& req) {
        try {
            auto body = json::parse(req.body);

            if (!body.contains("trader_id") || !body.contains("stock_id") ||
                !body.contains("side") || !body.contains("quantity") ||
                !body.contains("limit_price_paise")) {
                return makeErrorResponse(400, "Missing required fields: trader_id, stock_id, side, quantity, limit_price_paise");
            }

            int64_t trader_id = body["trader_id"].get<int64_t>();
            int64_t stock_id = body["stock_id"].get<int64_t>();
            std::string side_str = body["side"].get<std::string>();
            int64_t quantity = body["quantity"].get<int64_t>();
            int64_t limit_price_paise = body["limit_price_paise"].get<int64_t>();

            OrderSide side;
            try {
                side = stringToOrderSide(side_str);
            } catch (const std::exception& e) {
                return makeErrorResponse(400, "Invalid order side (must be BUY or SELL)");
            }

            auto result = engine.placeOrder(trader_id, stock_id, side, quantity, limit_price_paise);

            if (!result.success) {
                return makeErrorResponse(400, result.message);
            }

            json response = {
                {"success", true},
                {"message", result.message},
                {"order", orderToJson(*result.order)}
            };
            if (result.trade.has_value()) {
                response["trade"] = tradeToJson(*result.trade);
            } else {
                response["trade"] = nullptr;
            }

            return makeJsonResponse(201, response.dump());
        } catch (const json::exception& e) {
            return makeErrorResponse(400, "Invalid JSON request format: " + std::string(e.what()));
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Internal trading service error: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/orders - Retrieve orders with optional query filtering
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/orders").methods("GET"_method)
    ([&engine](const crow::request& req) {
        try {
            int limit = 100;
            if (auto l = req.url_params.get("limit")) {
                limit = std::atoi(l);
            }

            std::vector<Order> orders;
            if (auto t_id = req.url_params.get("trader_id")) {
                int64_t trader_id = std::atoll(t_id);
                orders = engine.getTraderOrders(trader_id, limit);
            } else {
                orders = engine.getAllOrders(limit);
            }

            json j_orders = json::array();
            for (const auto& ord : orders) {
                j_orders.push_back(orderToJson(ord));
            }

            return makeJsonResponse(200, j_orders.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Failed to retrieve orders: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/orders/<int> - Retrieve a specific order by ID
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/orders/<int>").methods("GET"_method)
    ([&engine](int order_id) {
        try {
            auto order_opt = engine.getOrderById(static_cast<int64_t>(order_id));
            if (!order_opt.has_value()) {
                return makeErrorResponse(404, "Order not found: #" + std::to_string(order_id));
            }
            return makeJsonResponse(200, orderToJson(*order_opt).dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving order: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // POST /api/orders/<int>/cancel - Cancel a pending order
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/orders/<int>/cancel").methods("POST"_method)
    ([&engine](int order_id) {
        try {
            auto result = engine.cancelOrder(static_cast<int64_t>(order_id));
            if (!result.success) {
                return makeErrorResponse(400, result.message);
            }

            json response = {
                {"success", true},
                {"message", result.message},
                {"order", orderToJson(*result.order)}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error processing cancellation: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/trades - Retrieve executed trade history
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/trades").methods("GET"_method)
    ([&engine](const crow::request& req) {
        try {
            int limit = 100;
            if (auto l = req.url_params.get("limit")) {
                limit = std::atoi(l);
            }

            std::optional<int64_t> order_id = std::nullopt;
            if (auto o_id = req.url_params.get("order_id")) {
                order_id = std::atoll(o_id);
            }

            auto trades = engine.getExecutionHistory(order_id, limit);

            json j_trades = json::array();
            for (const auto& tr : trades) {
                j_trades.push_back(tradeToJson(tr));
            }

            return makeJsonResponse(200, j_trades.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Failed to retrieve trades: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/portfolio?trader_id=<id> - Portfolio summary
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/portfolio").methods("GET"_method)
    ([&engine](const crow::request& req) {
        try {
            int64_t trader_id = 1; // Default to primary human trader (Lakshya)
            if (auto t_id = req.url_params.get("trader_id")) {
                trader_id = std::atoll(t_id);
            }

            auto summary_opt = engine.getTraderPortfolio(trader_id);
            if (!summary_opt.has_value()) {
                return makeErrorResponse(404, "Trader portfolio not found for ID: #" + std::to_string(trader_id));
            }

            auto snapshots = engine.getPortfolioSnapshots(trader_id);
            return makeJsonResponse(200, portfolioSummaryToJson(*summary_opt, snapshots).dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error computing portfolio summary: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // POST /api/portfolio/snapshot?trader_id=<id> - Persist real snapshot
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/portfolio/snapshot").methods("POST"_method)
    ([&engine](const crow::request& req) {
        try {
            int64_t trader_id = 1;
            if (auto t_id = req.url_params.get("trader_id")) {
                trader_id = std::atoll(t_id);
            }
            int64_t snap_id = engine.recordPortfolioSnapshot(trader_id);
            if (snap_id < 0) {
                return makeErrorResponse(404, "Trader not found to record snapshot");
            }
            json resp = {
                {"status", "success"},
                {"snapshot_id", snap_id},
                {"trader_id", trader_id}
            };
            return makeJsonResponse(201, resp.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error recording snapshot: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/positions?trader_id=<id> - Active stock positions
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/positions").methods("GET"_method)
    ([&engine](const crow::request& req) {
        try {
            int64_t trader_id = 1;
            if (auto t_id = req.url_params.get("trader_id")) {
                trader_id = std::atoll(t_id);
            }

            auto positions = engine.getTraderPositions(trader_id);

            json j_positions = json::array();
            for (const auto& p : positions) {
                j_positions.push_back(positionDetailToJson(p));
            }

            return makeJsonResponse(200, j_positions.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving positions: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/stocks - List all instruments
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/stocks").methods("GET"_method)
    ([&engine]() {
        try {
            auto stocks = engine.getAllStocks();
            auto trades = engine.getExecutionHistory();

            // Aggregate trade volume and price limits from database per stock
            std::unordered_map<int64_t, int64_t> stock_volume;
            std::unordered_map<int64_t, int64_t> stock_max_price;
            std::unordered_map<int64_t, int64_t> stock_min_price;

            for (const auto& tr : trades) {
                auto ord_opt = engine.getOrderById(tr.getOrderId());
                if (ord_opt.has_value()) {
                    int64_t s_id = ord_opt->getStockId();
                    stock_volume[s_id] += tr.getExecutedQuantity();
                    int64_t p = tr.getExecutedPricePaise();
                    if (stock_max_price.find(s_id) == stock_max_price.end() || p > stock_max_price[s_id]) {
                        stock_max_price[s_id] = p;
                    }
                    if (stock_min_price.find(s_id) == stock_min_price.end() || p < stock_min_price[s_id]) {
                        stock_min_price[s_id] = p;
                    }
                }
            }

            json j_stocks = json::array();
            for (const auto& s : stocks) {
                int64_t s_id = s.getId();
                int64_t tr_vol = stock_volume[s_id];
                int64_t max_p = stock_max_price[s_id];
                int64_t min_p = stock_min_price[s_id];
                auto market_data = computeStockMarketData(s, tr_vol, max_p, min_p);
                j_stocks.push_back(stockToJson(s, market_data));
            }
            return makeJsonResponse(200, j_stocks.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving stocks: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/stocks/<int>/history - Stock price history
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/stocks/<int>/history").methods("GET"_method)
    ([&engine](int stock_id) {
        try {
            auto stocks = engine.getAllStocks();
            const Stock* found_stock = nullptr;
            for (const auto& s : stocks) {
                if (s.getId() == static_cast<int64_t>(stock_id)) {
                    found_stock = &s;
                    break;
                }
            }
            if (!found_stock) {
                return makeErrorResponse(404, "Stock not found: #" + std::to_string(stock_id));
            }

            auto trades = engine.getExecutionHistory();
            int64_t tr_vol = 0;
            int64_t max_p = 0;
            int64_t min_p = 0;
            for (const auto& tr : trades) {
                auto ord_opt = engine.getOrderById(tr.getOrderId());
                if (ord_opt.has_value() && ord_opt->getStockId() == found_stock->getId()) {
                    tr_vol += tr.getExecutedQuantity();
                    int64_t p = tr.getExecutedPricePaise();
                    if (max_p == 0 || p > max_p) max_p = p;
                    if (min_p == 0 || p < min_p) min_p = p;
                }
            }

            auto market_data = computeStockMarketData(*found_stock, tr_vol, max_p, min_p);
            auto stock_json = stockToJson(*found_stock, market_data);
            json response = {
                {"stock_id", found_stock->getId()},
                {"symbol", found_stock->getSymbol()},
                {"current_price_paise", found_stock->getCurrentPricePaise()},
                {"current_price_inr", paiseToInr(found_stock->getCurrentPricePaise())},
                {"is_simulated", true},
                {"data_disclaimer", "Simulated intraday market history based on current price bounds and trades"},
                {"history", stock_json["history"]}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving stock history: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/traders - List all traders
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/traders").methods("GET"_method)
    ([&engine]() {
        try {
            auto traders = engine.getAllTraders();
            json j_traders = json::array();
            for (const auto& t : traders) {
                j_traders.push_back(traderToJson(*t));
            }
            return makeJsonResponse(200, j_traders.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving traders: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/alerts/stats - Aggregate Shield statistics across all alerts in SQLite
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/alerts/stats").methods("GET"_method)
    ([&alert_repo]() {
        try {
            auto stats = alert_repo.getStats();
            return makeJsonResponse(200, alertStatsToJson(stats).dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving alert statistics: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/alerts - Query Shield fraud & anomaly alerts with pagination
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/alerts").methods("GET"_method)
    ([&alert_repo](const crow::request& req) {
        try {
            int limit = 50;
            if (auto l = req.url_params.get("limit")) {
                limit = std::max(1, std::min(500, std::atoi(l)));
            }

            int page = 1;
            if (auto p = req.url_params.get("page")) {
                page = std::max(1, std::atoi(p));
            }

            int offset = (page - 1) * limit;
            if (auto off = req.url_params.get("offset")) {
                offset = std::max(0, std::atoi(off));
            }

            std::optional<int64_t> trader_id = std::nullopt;
            if (auto t_id = req.url_params.get("trader_id")) {
                trader_id = std::atoll(t_id);
            }

            std::optional<AlertSeverity> severity = std::nullopt;
            if (auto sev = req.url_params.get("severity")) {
                severity = stringToAlertSeverity(sev);
            }

            std::optional<AlertReviewStatus> review_status = std::nullopt;
            if (auto stat = req.url_params.get("review_status")) {
                review_status = stringToAlertReviewStatus(stat);
            }

            int64_t total_count = alert_repo.countByFilters(trader_id, severity, review_status);
            auto alerts = alert_repo.findByFilters(trader_id, severity, review_status, limit, offset);
            json j_alerts = json::array();
            for (const auto& a : alerts) {
                j_alerts.push_back(alertToJson(a));
            }

            int total_pages = total_count > 0 ? static_cast<int>((total_count + limit - 1) / limit) : 0;

            json response = {
                {"alerts", j_alerts},
                {"total_count", total_count},
                {"page", page},
                {"limit", limit},
                {"offset", offset},
                {"total_pages", total_pages}
            };

            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving alerts: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/alerts/<int> - Retrieve single alert by ID
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/alerts/<int>").methods("GET"_method)
    ([&alert_repo](int alert_id) {
        try {
            auto alert_opt = alert_repo.findById(static_cast<int64_t>(alert_id));
            if (!alert_opt.has_value()) {
                return makeErrorResponse(404, "Alert not found: #" + std::to_string(alert_id));
            }
            return makeJsonResponse(200, alertToJson(*alert_opt).dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving alert: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // POST /api/alerts/<int>/review - Update alert review status
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/alerts/<int>/review").methods("POST"_method)
    ([&alert_repo](const crow::request& req, int alert_id) {
        try {
            auto alert_opt = alert_repo.findById(static_cast<int64_t>(alert_id));
            if (!alert_opt.has_value()) {
                return makeErrorResponse(404, "Alert not found: #" + std::to_string(alert_id));
            }

            auto body = json::parse(req.body);
            if (!body.contains("status")) {
                return makeErrorResponse(400, "Missing required field: status (REVIEWED or CLEARED)");
            }

            std::string status_str = body["status"].get<std::string>();
            AlertReviewStatus new_status;
            try {
                new_status = stringToAlertReviewStatus(status_str);
            } catch (...) {
                return makeErrorResponse(400, "Invalid review status: must be REVIEWED or CLEARED");
            }

            bool updated = alert_repo.updateReviewStatus(static_cast<int64_t>(alert_id), new_status);
            if (!updated) {
                return makeErrorResponse(500, "Failed to update alert review status");
            }

            auto updated_alert = alert_repo.findById(static_cast<int64_t>(alert_id));
            json response = {
                {"success", true},
                {"message", "Alert status updated to " + status_str},
                {"alert", alertToJson(*updated_alert)}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const json::exception& e) {
            return makeErrorResponse(400, "Invalid JSON payload: " + std::string(e.what()));
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error reviewing alert: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/risk/<int> - Trader composite risk profile
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/risk/<int>").methods("GET"_method)
    ([&fraud_engine](int trader_id) {
        try {
            auto profile_opt = fraud_engine.getTraderRisk(static_cast<int64_t>(trader_id));
            if (!profile_opt.has_value()) {
                return makeErrorResponse(404, "Trader not found: #" + std::to_string(trader_id));
            }
            return makeJsonResponse(200, riskProfileToJson(*profile_opt).dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error calculating trader risk: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/risk - All traders risk ranking
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/risk").methods("GET"_method)
    ([&fraud_engine]() {
        try {
            auto profiles = fraud_engine.getAllTradersRisk();
            json j_profiles = json::array();
            for (const auto& p : profiles) {
                j_profiles.push_back(riskProfileToJson(p));
            }
            return makeJsonResponse(200, j_profiles.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving risk profiles: " + std::string(e.what()));
        }
    });
}

template void TradingController::registerRoutes<crow::SimpleApp>(
    crow::SimpleApp&, TradingEngine&, AlertRepository&, FraudEngine&);
template void TradingController::registerRoutes<crow::App<crow::CORSHandler>>(
    crow::App<crow::CORSHandler>&, TradingEngine&, AlertRepository&, FraudEngine&);

} // namespace tradeshield
