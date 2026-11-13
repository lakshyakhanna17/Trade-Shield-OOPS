# TradeShield — Development Plan & Milestones

This plan adheres strictly to the instructions in `AGENTS.md` (building in small stages, verifying with tests, and requiring approval before major architectural stages).

---

## Stage 1: System Design & Database Schema
- [x] Inspect project directory, requirements, and database schema.
- [x] Verify folder structure and missing dependencies/configurations.
- [x] Create and verify SQLite `schema.sql` and `seed.sql`.
- [x] Document system architecture (`docs/architecture.md`), including 4 OOP pillars.
- [x] Document REST API contracts (`docs/api_endpoints.md`).
- [x] Identify architectural issues, concurrency safety, and compiler requirements.
- [x] Architecture approved by user.

---

## Stage 2: Backend Foundation & Minimal Working Server (Completed)
- [x] Verify MSYS2 UCRT64 toolchain (GCC 16.2.0, CMake 4.4.5, Ninja 1.13.2).
- [x] Install required dependencies (`asio`, `nlohmann-json`, vendored Crow C++).
- [x] Establish modular backend directory tree (`src/api`, `src/models`, `src/database`, `src/engine`, `src/detection`, `src/simulation`, `tests`).
- [x] Create `CMakeLists.txt` configured for C++17, Ninja, and Windows networking.
- [x] Create `src/config.hpp` for environment and port configuration.
- [x] Implement minimal Crow HTTP server with `GET /api/health` returning JSON.
- [x] Configure and build with CMake & Ninja.
- [x] Run and verify HTTP endpoint locally (`http://127.0.0.1:8080/api/health`).

---

## Stage 3: Domain Models & Database Layer (Completed)
- [x] Implement domain models (`Trader`, `HumanTrader`, `BotTrader`, `Stock`, `Order`, `Trade`, `Position`, `Portfolio`, `Alert`).
- [x] Implement monetary validation using integer paise and overflow detection.
- [x] Implement `DatabaseManager` (SQLite connection, WAL mode, transaction RAII, thread-safe mutex).
- [x] Implement `Statement` RAII wrapper for prepared statements.
- [x] Implement database repositories (`StockRepository`, `TraderRepository`, `OrderRepository`, `TradeRepository`, `PositionRepository`, `PortfolioRepository`, `AlertRepository`).
- [x] Write and run 15 comprehensive unit tests (all passing with 0 failures).
- [x] Verify `/api/health` continues to work.

---

## Stage 4: Trading Engine & Business Rules (Completed)
- [x] Implement `OrderValidator` (validate trader, stock, positive qty/price, cash balance for BUY, position holding for SELL, integer overflow protection).
- [x] Implement `TradingEngine` (deterministic matching, marketable immediate execution, non-marketable pending orders, cancellation lifecycle).
- [x] Atomic transactions with `TransactionGuard` (guaranteeing all-or-nothing execution across cash balances, stock positions, orders, trades, and portfolios).
- [x] Comprehensive portfolio calculation service (`PortfolioSummary` with unrealized P&L and moving weighted average buy price).
- [x] Implement Crow REST API endpoints (`/api/orders`, `/api/trades`, `/api/portfolio`, `/api/positions`, `/api/stocks`, `/api/traders`).
- [x] Write and run 21 comprehensive tests (100% passing, 0 failures).
- [x] Live end-to-end HTTP verification of all endpoints.

---

## Stage 5: Shield Fraud & Anomaly Detection Engine (Completed)
- [x] Implement `IDetectionRule` interface and `BaseDetectionRule` with ISO-8601 epoch time arithmetic.
- [x] Implement `HighVolumeRule` (differentiates isolated large order = LOW risk vs repeated large orders / 3x volume spikes = MEDIUM/HIGH risk).
- [x] Implement `RepeatedCancellationRule` (tracks sliding cancellation ratio and rapid cancellation frequency = MEDIUM/HIGH risk).
- [x] Implement `HighFrequencyRule` (detects rapid order submission bursts within tight sliding windows = MEDIUM/HIGH risk).
- [x] Implement `RiskScorer` (composite risk calculation: `max_score + 0.25 * sum_others`, 0–100 clamping, severity classification bands).
- [x] Implement `FraudEngine` coordinator (thread-safe polymorphic rule execution, duplicate prevention, exception isolation).
- [x] Extend `AlertRepository` with multi-criteria dynamic filtering (`findByFilters`) and duplicate checks (`existsByOrderAndRule`).
- [x] Integrate post-commit trading event pipeline into `TradingEngine` (`emitTradingEvent`).
- [x] Add Crow REST API routes (`GET /api/alerts`, `GET /api/alerts/:id`, `POST /api/alerts/:id/review`, `GET /api/risk/:traderId`, `GET /api/risk`).
- [x] Write and run 18 new unit and integration tests (39 total across test suite, 100% passing, 0 failures).
- [x] Verify live end-to-end event emission, alert persistence, risk score updating, and alert status review.

---

## Stage 6: Bot Simulation Engine (Completed)
- [x] Implement `IBotStrategy` interface defining discrete algorithmic actions (`executeTick`).
- [x] Implement `NormalBotStrategy` (modest retail order sizes, bounded quantities, low risk profile).
- [x] Implement `SuspiciousBotStrategy` (large order quantities $\ge 100$ shares, repeated volume bursts, triggers `HighVolumeRule`).
- [x] Implement `RapidCancelBotStrategy` (submits non-marketable pending limit orders and systematically cancels them, triggers `RepeatedCancellationRule`).
- [x] Implement `RandomBotStrategy` (bounded pseudo-random choices with deterministic seed support for testing).
- [x] Verify all bot actions route strictly through `TradingEngine::placeOrder()` and `TradingEngine::cancelOrder()`.
- [x] Implement `SimulationManager` (background thread worker with `std::atomic<bool>`, `std::condition_variable`, thread-safe lifecycle, metrics tracking).
- [x] Expose Crow REST API endpoints (`POST /api/simulation/start`, `POST /api/simulation/stop`, `GET /api/simulation/status`, `POST /api/simulation/reset`, `POST /api/simulation/tick`).
- [x] Write and run 11 new automated unit and integration tests (50 total across test suite, 100% passing, 0 failures).
- [x] Verify live end-to-end bot execution, automated alert generation, and risk escalation.

---

## Stage 7: React Frontend Dashboard (Next Stage)
- [ ] Setup Vite environment, layout, and routing (`react-router-dom`).
- [ ] Implement reusable UI components (Fintech dark-theme styling, metric cards, badges).
- [ ] Build pages:
  - **Dashboard**: Market overview, quick stats, recent alerts.
  - **Market & Trading**: Stock ticker cards, interactive order placement form, order book/history.
  - **Portfolio**: Holdings table, cash balance, P&L calculations.
  - **Shield Monitor**: Real-time fraud alert feed, risk score gauges, review status toggle.
  - **Simulation Control**: Bot activity triggers, tick rate, start/stop toggle.
- [ ] Integrate Recharts for stock price history, portfolio allocation, and risk distributions.

---

## Stage 8: End-to-End Verification & Documentation Finalization
- [ ] End-to-end integration test (frontend <-> backend API <-> SQLite DB).
- [ ] Verify all requirements in `PROJECT_REQUIREMENTS.md` and `AGENTS.md`.
- [ ] Finalize documentation in `README.md`.
