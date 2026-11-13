# TradeShield — Backend & System Architecture

## 1. System Overview
TradeShield is a simulated stock-market trading platform integrated with a real-time fraud and anomaly detection engine (Shield Engine). It enables human traders and automated simulation bots to trade Indian equities (NSE) while dynamically monitoring and scoring anomalous order behaviors.

```
+------------------------------------------------------------------+
|                     React + Vite Frontend                        |
|  (Market Dashboard, Order Form, Trader Portfolio, Shield Monitor) |
+---------------------------------+--------------------------------+
                                  | HTTP / JSON REST APIs
+---------------------------------v--------------------------------+
|                     Crow C++17 HTTP Server                       |
|               (Controllers & Route Handlers)                     |
+---------------------------------+--------------------------------+
                                  |
            +---------------------+---------------------+
            |                                           |
+-----------v------------+                  +-----------v------------+
|     Trading Engine     |                  |   Simulation Manager   |
|  - Order Validation    |                  |  - Normal Bot          |
|  - Execution Matching  |                  |  - Suspicious Bot      |
|  - Position / Cash RAII|                  |  - Random Bot          |
+-----------+------------+                  |  - Rapid Cancel Bot    |
            |                               +-----------+------------+
            | (Uniform Order Pipeline)                  |
            +---------------------+---------------------+
                                  |
+---------------------------------v--------------------------------+
|                    Shield Detection Engine                       |
|  - High Volume Anomaly Rule                                      |
|  - Repeated Cancellation Rule                                    |
|  - High Frequency Anomaly Rule                                   |
|  - Risk Scorer (0-100 normalized score)                          |
+---------------------------------+--------------------------------+
                                  |
+---------------------------------v--------------------------------+
|                 Database Layer (SQLite + WAL)                    |
|       (Connection RAII, Mutex-Locked, Repositories)              |
+------------------------------------------------------------------+
```

---

## 2. Demonstration of the Four OOP Pillars

### 2.1 Encapsulation
- **Financial Balances & Positions**: Cash balances and share quantities are strictly encapsulated within `Trader` and `Position` domain objects. Direct attribute mutation is prohibited; transitions occur through guarded methods (`debitCash`, `creditCash`, `addShares`, `deductShares`) enforcing invariant checks (e.g., non-negative cash balances, ownership checks).
- **Database & Resource Management**: The SQLite handle (`sqlite3*`) and synchronization primitives (`std::mutex`) are encapsulated within `DatabaseManager`. Callers interact via high-level repository interfaces and RAII transaction scopes (`TransactionGuard`).
- **Order Lifecycle**: Order status transitions (`PENDING -> EXECUTED`, `PENDING -> CANCELLED`) are encapsulated within `Order` and `TradingEngine`, preventing invalid intermediate states.

### 2.2 Inheritance
- **Trader Hierarchy**:
  - `Trader` (Base class containing common identity, ledger, and cash properties)
    - `HumanTrader` (Interactive user profile)
    - `BotTrader` (Automated agent driven by strategy pattern)
- **Detection Rules Hierarchy**:
  - `BaseDetectionRule` (Base abstract class providing time-window tracking and threshold evaluation)
    - `HighVolumeRule` (Detects volume spikes relative to average order volume)
    - `RepeatedCancellationRule` (Detects rapid cancel patterns / spoofing)
    - `HighFrequencyRule` (Detects high-frequency submission bursts)
- **Bot Strategies**:
  - `IBotStrategy` (Base abstract strategy)
    - `NormalBotStrategy`
    - `SuspiciousBotStrategy`
    - `RapidCancelBotStrategy`
    - `RandomBotStrategy`

### 2.3 Abstraction
- **IDetectionRule Interface**: Defines the pure virtual contract:
  ```cpp
  class IDetectionRule {
  public:
      virtual ~IDetectionRule() = default;
      virtual std::string getRuleName() const = 0;
      virtual std::optional<Alert> evaluate(const OrderContext& ctx) = 0;
  };
  ```
- **IBotStrategy Interface**:
  ```cpp
  class IBotStrategy {
  public:
      virtual ~IBotStrategy() = default;
      virtual std::string getStrategyName() const = 0;
      virtual std::optional<OrderRequest> generateOrder(const MarketSnapshot& market, const Trader& bot) = 0;
  };
  ```
- The `FraudEngine` and `SimulationManager` interact exclusively through these abstract interfaces, remaining completely decoupled from concrete detection heuristics or trading bot algorithms.

### 2.4 Polymorphism
- **Dynamic Rule Dispatch**: `FraudEngine` maintains a collection `std::vector<std::unique_ptr<IDetectionRule>> rules_`. When an order event is emitted, it iterates through each rule and polymorphically invokes `rule->evaluate(ctx)` via virtual method dispatch.
- **Dynamic Strategy Execution**: Each `BotTrader` owns an `std::unique_ptr<IBotStrategy>`. During the simulation cycle, `bot->generateOrder(market)` polymorphically delegates to the assigned concrete strategy.
- **Extensibility**: New fraud detection heuristics or trading strategies can be added without modifying the core engine or server routes (Open-Closed Principle).

---

## 3. Modular Directory Structure

```
backend/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── config.hpp
│   ├── models/
│   │   ├── Trader.hpp
│   │   ├── Stock.hpp
│   │   ├── Order.hpp
│   │   ├── Trade.hpp
│   │   ├── Position.hpp
│   │   ├── Alert.hpp
│   │   └── RiskSummary.hpp
│   ├── database/
│   │   ├── DatabaseManager.hpp
│   │   ├── DatabaseManager.cpp
│   │   ├── repositories/
│   │   │   ├── TraderRepository.hpp
│   │   │   ├── StockRepository.hpp
│   │   │   ├── OrderRepository.hpp
│   │   │   ├── PositionRepository.hpp
│   │   │   └── AlertRepository.hpp
│   ├── engine/
│   │   ├── OrderValidator.hpp
│   │   ├── OrderValidator.cpp
│   │   ├── TradingEngine.hpp
│   │   └── TradingEngine.cpp
│   ├── detection/
│   │   ├── IDetectionRule.hpp
│   │   ├── BaseDetectionRule.hpp
│   │   ├── BaseDetectionRule.cpp
│   │   ├── DetectionContext.hpp
│   │   ├── DetectionResult.hpp
│   │   ├── TradingEvent.hpp
│   │   ├── HighVolumeRule.hpp
│   │   ├── HighVolumeRule.cpp
│   │   ├── RepeatedCancellationRule.hpp
│   │   ├── RepeatedCancellationRule.cpp
│   │   ├── HighFrequencyRule.hpp
│   │   ├── HighFrequencyRule.cpp
│   │   ├── RiskScorer.hpp
│   │   ├── RiskScorer.cpp
│   │   ├── FraudEngine.hpp
│   │   └── FraudEngine.cpp
│   ├── simulation/
│   │   ├── IBotStrategy.hpp
│   │   ├── NormalBotStrategy.hpp
│   │   ├── NormalBotStrategy.cpp
│   │   ├── SuspiciousBotStrategy.hpp
│   │   ├── SuspiciousBotStrategy.cpp
│   │   ├── RapidCancelBotStrategy.hpp
│   │   ├── RapidCancelBotStrategy.cpp
│   │   ├── RandomBotStrategy.hpp
│   │   ├── RandomBotStrategy.cpp
│   │   ├── SimulationManager.hpp
│   │   └── SimulationManager.cpp
│   └── api/
│       ├── JsonAdapters.hpp
│       ├── TradingController.hpp
│       ├── TradingController.cpp
│       ├── SimulationController.hpp
│       └── SimulationController.cpp
└── tests/
    ├── TestFramework.hpp
    ├── main.cpp
    ├── test_domain_models.cpp
    ├── test_database.cpp
    ├── test_trading_engine.cpp
    ├── test_fraud_engine.cpp
    └── test_simulation.cpp
```

---

## 4. Key Architectural Decisions & Safeguards
1. **Integer Paise Representation**: All prices, values, and balances are stored and computed in integer paise (1 Rupee = 100 paise) using `int64_t` to completely prevent floating-point inaccuracies. Conversion to Rupees is performed strictly for frontend display.
2. **Unified Order Processing Pipeline**: Bot orders and human orders follow the identical processing route: `OrderRequest -> OrderValidator -> FraudEngine -> ExecutionEngine -> SQLite Ledger Update`.
3. **Thread-Safe Database Access**: SQLite operates in WAL mode (`PRAGMA journal_mode = WAL;`) and all C++ write transactions are guarded by `std::mutex` inside `DatabaseManager`.
4. **Explainable Fraud Scoring**: Alerts record `rule_name`, `severity` (LOW/MEDIUM/HIGH), `risk_points` (0–100), and a clear natural-language explanation.
