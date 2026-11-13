#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "engine/TradingEngine.hpp"
#include "database/repositories/AlertRepository.hpp"
#include "detection/FraudEngine.hpp"

namespace tradeshield {

/**
 * @brief REST API Controller mapping HTTP requests to TradingEngine operations.
 * Thin controller ensuring clean separation between Crow web routing and trading domain logic.
 */
class TradingController {
public:
    template<typename AppType>
    static void registerRoutes(AppType& app,
                               TradingEngine& engine,
                               AlertRepository& alert_repo,
                               FraudEngine& fraud_engine);

private:
    static crow::response makeJsonResponse(int code, const std::string& body);
    static crow::response makeErrorResponse(int code, const std::string& error_msg);
};

} // namespace tradeshield
