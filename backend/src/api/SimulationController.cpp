#include "SimulationController.hpp"
#include "JsonAdapters.hpp"
#include <iostream>

namespace tradeshield {

crow::response SimulationController::makeJsonResponse(int code, const std::string& body) {
    crow::response res(code, body);
    res.set_header("Content-Type", "application/json");
    return res;
}

crow::response SimulationController::makeErrorResponse(int code, const std::string& error_msg) {
    json j = {
        {"success", false},
        {"error", error_msg}
    };
    return makeJsonResponse(code, j.dump());
}

template<typename AppType>
void SimulationController::registerRoutes(AppType& app, SimulationManager& sim_manager) {
    // ----------------------------------------------------
    // POST /api/simulation/start - Start automated bot simulation
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/simulation/start").methods("POST"_method)
    ([&sim_manager](const crow::request& req) {
        try {
            int tick_interval_ms = 2000;
            std::vector<int64_t> bot_ids;

            if (!req.body.empty()) {
                try {
                    auto body = json::parse(req.body);
                    if (body.contains("tick_interval_ms")) {
                        tick_interval_ms = body["tick_interval_ms"].get<int>();
                        if (tick_interval_ms < 50) {
                            return makeErrorResponse(400, "tick_interval_ms must be at least 50 ms");
                        }
                    }
                    if (body.contains("bot_ids") && body["bot_ids"].is_array()) {
                        for (const auto& item : body["bot_ids"]) {
                            bot_ids.push_back(item.get<int64_t>());
                        }
                    }
                } catch (const json::exception& e) {
                    return makeErrorResponse(400, "Invalid JSON payload format: " + std::string(e.what()));
                }
            }

            if (sim_manager.isRunning()) {
                return makeErrorResponse(400, "Simulation is already running");
            }

            bool started = sim_manager.start(tick_interval_ms, bot_ids);
            if (!started) {
                return makeErrorResponse(500, "Failed to start simulation thread");
            }

            auto status = sim_manager.getStatus();
            json response = {
                {"success", true},
                {"message", "Simulation started successfully"},
                {"is_running", true},
                {"tick_interval_ms", status.tick_interval_ms},
                {"active_bots_count", status.active_bots.size()}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Internal simulation service error: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // POST /api/simulation/stop - Stop automated bot simulation
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/simulation/stop").methods("POST"_method)
    ([&sim_manager]() {
        try {
            bool was_running = sim_manager.isRunning();
            sim_manager.stop();

            auto status = sim_manager.getStatus();
            json response = {
                {"success", true},
                {"message", was_running ? "Simulation stopped successfully" : "Simulation was not running"},
                {"is_running", false},
                {"total_orders_generated", status.total_orders_generated},
                {"total_executions_generated", status.total_executions_generated},
                {"total_cancellations_generated", status.total_cancellations_generated}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error stopping simulation: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // GET /api/simulation/status - Query current simulation state & metrics
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/simulation/status").methods("GET"_method)
    ([&sim_manager]() {
        try {
            auto status = sim_manager.getStatus();
            return makeJsonResponse(200, simulationStatusToJson(status).dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error retrieving simulation status: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // POST /api/simulation/reset - Reset simulation performance counters
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/simulation/reset").methods("POST"_method)
    ([&sim_manager]() {
        try {
            sim_manager.resetMetrics();
            json response = {
                {"success", true},
                {"message", "Simulation performance metrics reset successfully. Database records preserved."}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error resetting simulation metrics: " + std::string(e.what()));
        }
    });

    // ----------------------------------------------------
    // POST /api/simulation/tick - Manually advance a single tick (testing / debugging)
    // ----------------------------------------------------
    CROW_ROUTE(app, "/api/simulation/tick").methods("POST"_method)
    ([&sim_manager]() {
        try {
            auto results = sim_manager.stepTick();
            json actions_json = json::array();
            for (const auto& r : results) {
                json action_item = {
                    {"action_type", botActionTypeToString(r.action_type)},
                    {"success", r.success},
                    {"message", r.message}
                };
                if (r.order.has_value()) {
                    action_item["order_id"] = r.order->getId();
                }
                actions_json.push_back(action_item);
            }

            json response = {
                {"success", true},
                {"actions_executed", results.size()},
                {"results", actions_json}
            };
            return makeJsonResponse(200, response.dump());
        } catch (const std::exception& e) {
            return makeErrorResponse(500, "Error executing simulation tick: " + std::string(e.what()));
        }
    });
}

template void SimulationController::registerRoutes<crow::SimpleApp>(
    crow::SimpleApp&, SimulationManager&);
template void SimulationController::registerRoutes<crow::App<crow::CORSHandler>>(
    crow::App<crow::CORSHandler>&, SimulationManager&);

} // namespace tradeshield
