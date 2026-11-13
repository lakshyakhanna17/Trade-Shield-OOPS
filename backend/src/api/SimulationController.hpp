#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "simulation/SimulationManager.hpp"

namespace tradeshield {

/**
 * @brief REST API Controller mapping HTTP requests to SimulationManager operations.
 * Exposes endpoints to start, stop, monitor, and reset the automated bot simulation.
 */
class SimulationController {
public:
    template<typename AppType>
    static void registerRoutes(AppType& app, SimulationManager& sim_manager);

private:
    static crow::response makeJsonResponse(int code, const std::string& body);
    static crow::response makeErrorResponse(int code, const std::string& error_msg);
};

} // namespace tradeshield
