cmake_minimum_required(VERSION 3.15)
project(TradeShieldBackend LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Crow cmake module path for Findasio.cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow/cmake)

# Crow configuration
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/crow)

# Find dependencies
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# Core domain models, database and trading engine library
add_library(tradeshield_core STATIC
    src/models/Trader.cpp
    src/models/HumanTrader.cpp
    src/models/BotTrader.cpp
    src/models/Stock.cpp
    src/models/Order.cpp
    src/models/Trade.cpp
    src/models/Position.cpp
    src/models/Portfolio.cpp
    src/models/Alert.cpp
    src/database/Statement.cpp
    src/database/DatabaseManager.cpp
    src/database/repositories/TraderRepository.cpp
    src/database/repositories/StockRepository.cpp
    src/database/repositories/OrderRepository.cpp
    src/database/repositories/TradeRepository.cpp
    src/database/repositories/PositionRepository.cpp
    src/database/repositories/PortfolioRepository.cpp
    src/database/repositories/AlertRepository.cpp
    src/engine/OrderValidator.cpp
    src/engine/TradingEngine.cpp
    src/detection/BaseDetectionRule.cpp
    src/detection/HighVolumeRule.cpp
    src/detection/RepeatedCancellationRule.cpp
    src/detection/HighFrequencyRule.cpp
    src/detection/RiskScorer.cpp
    src/detection/FraudEngine.cpp
    src/simulation/NormalBotStrategy.cpp
    src/simulation/SuspiciousBotStrategy.cpp
    src/simulation/RapidCancelBotStrategy.cpp
    src/simulation/RandomBotStrategy.cpp
    src/simulation/SimulationManager.cpp
    src/api/TradingController.cpp
    src/api/SimulationController.cpp
)

target_include_directories(tradeshield_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(tradeshield_core PUBLIC
    Crow::Crow
    SQLite::SQLite3
    Threads::Threads
    nlohmann_json::nlohmann_json
)

if (WIN32)
    target_link_libraries(tradeshield_core PUBLIC ws2_32 mswsock)
endif()

# Server executable
add_executable(tradeshield_server
    src/main.cpp
)

target_include_directories(tradeshield_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(tradeshield_server PRIVATE
    tradeshield_core
)

# Test runner executable
add_executable(tradeshield_tests
    tests/main.cpp
    tests/test_domain_models.cpp
    tests/test_database.cpp
    tests/test_trading_engine.cpp
    tests/test_fraud_engine.cpp
    tests/test_simulation.cpp
)

target_include_directories(tradeshield_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(tradeshield_tests PRIVATE
    tradeshield_core
)

enable_testing()
add_test(NAME AllTests COMMAND tradeshield_tests)
