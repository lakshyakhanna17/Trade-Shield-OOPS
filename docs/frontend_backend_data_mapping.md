# TradeShield — Frontend–Backend Data Integration Mapping

**Date:** 2026-11-12
**Purpose:** Comprehensive audit of every displayed metric, chart, and component across the 7 application pages. Defines the single source of truth (C++ backend + SQLite), identifies current mock/static data usages, and specifies required corrections.

---

## 1. System-Wide Architecture & Truth Rules

1. **Persistent Store:** SQLite (`database/tradeshield.db`) is the sole persistent store.
2. **Business Engine:** C++17 Core (`TradingEngine`, `FraudEngine`, `SimulationManager`) manages all trading rules, order matching, alert generation, and simulation state.
3. **Paise Convention:** All financial values are calculated in integer paise (`₹1.00 = 100 paise`) by the backend to eliminate floating-point rounding errors.
4. **No Silent Mock Fallbacks:** In LIVE mode, API call failures must never silently fall back to `mockData.js`. If the backend is disconnected, an explicit error/disconnected state must be presented.
5. **Mock Data Isolation:** `mockData.js` is preserved strictly for explicitly selected standalone/demo mode.

---

## 2. Component-by-Component Data Mapping

### 2.1 Page 1: Dashboard (`/`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Hero Header** | "Last update: 2 min ago", engine status indicator | Hardcoded JSX string in `Dashboard.jsx` | `GET /api/simulation/status` (`last_tick_time`) or dynamic fetch timestamp | **YES** (Static string "2 min ago") | No | Bind dynamically to `last_tick_time` or latest refresh timestamp (`formatRelativeTime`). |
| **Top Stock Cards** (`StockCard.jsx`) | Symbol, Company Name, Current Price (₹), 24h Change (%) | `stocks` state in `TradeShieldProvider` | `GET /api/stocks` | No (from backend), but initial state used `MOCK_STOCKS` | Yes (polled on sim tick) | Ensure initial state doesn't show mock data before load; ensure price/change directly from `current_price_paise` and `change_24h`. |
| **Top Stock Sparkline** (`StockCard.jsx`) | Intraday trend area chart | `stock.history` | `GET /api/stocks` or `GET /api/stocks/{id}/history` | **YES** (Simulated inside React `normalizeStocks`) | Yes (recomputed on price change) | Backend must provide consistent price history or simulated history endpoint. |
| **Account Valuation** (`PortfolioChart.jsx`) | Total Account Value (₹), Cash Balance (₹), Invested Value (₹) | `portfolio` state | `GET /api/portfolio?trader_id=1` | No (from backend), but initial state used `MOCK_PORTFOLIO` | Yes (polled on sim tick & order executions) | Initialize cleanly; rely solely on `GET /api/portfolio` without mock fallback. |
| **Unrealized P&L** (`PortfolioChart.jsx`) | Total Unrealized P&L (₹), P&L (%) | `portfolio.total_unrealized_pnl`, `portfolio.total_pnl_percent` | `GET /api/portfolio?trader_id=1` | No (calculated by C++ backend) | Yes | Ensure no fallback to `MOCK_PORTFOLIO` on network error. |
| **Portfolio Trajectory Chart** (`PortfolioChart.jsx`) | 7-day valuation area chart | `portfolio.performance_history` | `GET /api/portfolio` | **YES** (Hardcoded `MOCK_PORTFOLIO.performance_history` in `normalizePortfolio`) | No (static mock array) | Implement backend performance history in `GET /api/portfolio` based on trader balance and trade executions. |
| **Recent Alerts Feed** (`RecentAlertsWidget.jsx`) | Top 4 open alerts: Rule, Symbol, Trader, Description, Risk Pts, Time | `alerts` state | `GET /api/alerts?limit=50` | No (persisted in SQLite `alerts`) | Yes (polled every 1.8s) | Enrich symbol from associated order; ensure empty state renders gracefully when no alerts exist. |
| **Simulation Quick Widget** (`SimulationStatusWidget.jsx`) | Running state, Total Ticks, Orders, Cancellations, Start/Stop/Reset | `simulation` state | `GET /api/simulation/status`, `POST /api/simulation/start`, `POST /api/simulation/stop`, `POST /api/simulation/reset` | No (real-time from C++ `SimulationManager`) | Yes (polled every 1.8s) | Ensure buttons reflect `isSimActionLoading` and no fallback to `MOCK_SIMULATION_STATUS`. |
| **Market Overview Table** (`MarketTable.jsx`) | Stock Symbol, Name, Price, 24h %, 7d %, Volume, Shield Risk | `stocks` state | `GET /api/stocks` | Partial (`7d %` and `shield_risk` defaulted from `MOCK_STOCKS`) | Yes | Calculate `shield_risk` dynamically from active alerts per stock/trader; backend provides 24h % and trade-aggregated volume. |

---

### 2.2 Page 2: Market Overview (`/market`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Market Header & Filters** | Search query, Filter pills (All, Gainers, Losers, High Vol) | Local state + `stocks` | `GET /api/stocks` | No | Yes | Filters apply on live backend prices and trade volumes. |
| **Selected Stock Summary** | Last Price, 24h High, 24h Low, Total Volume, 24h Change (%) | `selectedStock` from `stocks` | `GET /api/stocks` | No (computed in C++ `computeStockMarketData`) | Yes | Verified mathematically: `low_24h_paise <= current_price_paise <= high_24h_paise`. |
| **Selected Stock Chart** | Area chart for selected stock | `selectedStock.history` | `GET /api/stocks` / `GET /api/stocks/{id}/history` | **YES** (Synthesized in React `normalizeStocks`) | Updates on price change | Backend endpoint `GET /api/stocks/{id}/history` or history array in `GET /api/stocks` backed by actual SQLite trades and baseline open. |
| **All Stocks Sidebar List** | Symbol, Name, Price, Change pill | `stocks` list | `GET /api/stocks` | No | Yes | Remove fallback to `MOCK_STOCKS`. |

---

### 2.3 Page 3: Trading Terminal (`/trading`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Stock Selector** | Dropdown of tradable instruments | `stocks` state | `GET /api/stocks` | No | Yes | Source directly from backend stocks. |
| **Available Cash** | Available Cash Balance (₹) | `portfolio.available_cash` | `GET /api/portfolio?trader_id=1` | No | Yes | Source directly from SQLite `traders.cash_balance_paise`. |
| **Order Form Input** | Side (BUY/SELL), Quantity, Limit Price, Estimated Total | User input | N/A (Client input) | No | User driven | Validates integer paise and cash availability before submission. |
| **Order Submission** | Submit Limit Order | `placeOrder()` | `POST /api/orders` | No | On submit | Calls real C++ `TradingEngine::placeOrder`. Returns real execution/rejection messages. NEVER mock on error. |
| **Order Cancellation** | Cancel Pending Order | `cancelOrder()` | `POST /api/orders/{id}/cancel` | No | On submit | Calls real C++ `TradingEngine::cancelOrder`. Updates SQLite state atomically. |
| **Recent Orders / Book** | Order ID, Side, Qty, Price, Status | `orders` state | `GET /api/orders?trader_id=1` | No (from SQLite `orders`) | Yes | Display live orders from database without mock fallback. |

---

### 2.4 Page 4: Portfolio & Holdings (`/portfolio`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Total Account Value** | Cash + Mark-to-Market holdings | `portfolio.total_account_value` | `GET /api/portfolio?trader_id=1` | No | Yes | Exact paise calculation from backend. |
| **Invested Value** | Total principal cost of active holdings | `portfolio.invested_value` | `GET /api/portfolio?trader_id=1` | No | Yes | Calculated from SQLite `positions.average_buy_price_paise * quantity`. |
| **Available Cash** | Cash balance ready for execution | `portfolio.available_cash` | `GET /api/portfolio?trader_id=1` | No | Yes | Direct from SQLite `traders.cash_balance_paise`. |
| **Unrealized P&L** | Mark-to-market gain/loss in ₹ and % | `portfolio.total_unrealized_pnl`, `portfolio.total_pnl_percent` | `GET /api/portfolio?trader_id=1` | No | Yes | `holdings_value - invested_value` in integer paise. |
| **Holdings Allocation Chart** | Donut chart of holdings by valuation | `holdings.map(...)` | `GET /api/positions?trader_id=1` | No | Yes | Sourced directly from active positions in SQLite. |
| **Stock Holdings Table** | Symbol, Name, Quantity, Avg Buy Price, Current Price, Current Value, P&L (₹), P&L (%) | `holdings` state | `GET /api/positions?trader_id=1` | No | Yes | Remove fallback to `MOCK_HOLDINGS`. Empty state when trader owns 0 stocks. |

---

### 2.5 Page 5: Shield Monitor (`/shield`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Active Incidents** | Open incident count | `alertStats.open_alerts` | `GET /api/alerts/stats` | No | Yes | Calculated directly in SQLite `count(*) where review_status='OPEN'`. |
| **High/Critical Severity** | High severity incident count | `alertStats.high_severity_alerts` | `GET /api/alerts/stats` | No | Yes | Direct SQLite aggregation. |
| **Total Alerts** | Lifetime alerts count | `alertStats.total_alerts` | `GET /api/alerts/stats` | No | Yes | Direct SQLite count across all historical alerts. |
| **Reviewed/Cleared** | Resolved alerts count | `alertStats.reviewed_alerts` | `GET /api/alerts/stats` | No | Yes | Direct SQLite count. |
| **Detection Rules Catalogue** | Rule metadata, description, thresholds | Static definitions | None (Static domain specifications) | Expected | Static | Accurate documentation of C++ rule classes (`HighVolumeRule`, `RepeatedCancellationRule`, `HighFrequencyRule`). |
| **Trader Risk Leaderboard** | Trader Name, Type, Score (0-100), Risk Tier, Triggered Rules | `riskProfiles` state | `GET /api/risk` | **PARTIAL** (Fell back to `fallbackTraders` mock array if empty) | Yes | Remove `fallbackTraders` mock array in live mode; populate exclusively from `GET /api/risk`. |
| **Alert Incidents Table** | Alert ID, Rule Name, Trader, Symbol, Severity, Risk Pts, Status, Occurrences, Timestamp | `alerts` state | `GET /api/alerts?limit=50&page=1` | No | Yes | Sourced from SQLite `alerts`. |
| **Alert Review / Clear Action** | Change status from OPEN to REVIEWED/CLEARED | `handleReview(alertId)` | `POST /api/alerts/{id}/review` | **BUG FOUND** | On click | **CRITICAL BUG FIXED:** `Shield.jsx` was sending comment string as `status`, causing 400. Fix to send `status: 'REVIEWED'`. |
| **Alert Details Modal** | Full evidence, occurrence count, first/last seen timestamps | `selectedAlert` state | `GET /api/alerts/{id}` or table row | No | Modal open | Sourced from SQLite incident aggregation fields. |

---

### 2.6 Page 6: Bot Simulation Center (`/simulation`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Bot Strategy Cards** | AlphaBot, ShadowBot, FlashCancel, StochasticBot | Static profile definitions | C++ Strategy definitions | Expected | Static metadata | Match C++17 implementations (`NormalBotStrategy`, `SuspiciousBotStrategy`, `RapidCancelBotStrategy`, `RandomBotStrategy`). |
| **Simulation Status Banner** | RUNNING / STOPPED, tick interval | `simulation.is_running`, `simulation.tick_interval_ms` | `GET /api/simulation/status` | No | Yes | Real-time status from C++ `SimulationManager`. |
| **Simulation Controls** | Start, Stop, Reset | Context handlers | `POST /api/simulation/start`, `POST /api/simulation/stop`, `POST /api/simulation/reset` | No | On click | Strictly calls real C++ server. Locked during in-flight operations. |
| **Telemetry Counters** | Total Ticks, Orders, Executions, Cancellations, Rejections, System Faults | `simulation.*` | `GET /api/simulation/status` | No | Yes (polled every 1.8s) | Sourced directly from `SimulationManager::getStatus()`. |

---

### 2.7 Page 7: Transactions & Audit (`/transactions`)

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Orders Tab** | Order ID, Trader Name, Symbol, Side, Type, Quantity, Limit Price (₹), Status, Timestamp | `orders` state | `GET /api/orders?limit=100` | No | Yes | Enriched with trader name and symbol from SQLite. Remove fallback to `MOCK_ORDERS`. |
| **Order Cancellation** | Cancel button on PENDING orders | `cancelOrder()` | `POST /api/orders/{id}/cancel` | No | On click | Atomic SQLite cancellation via C++ `TradingEngine`. |
| **Executed Trades Tab** | Trade ID, Order ID, Symbol, Buyer, Seller, Quantity, Execution Price (₹), Trade Value (₹), Executed At | `trades` state | `GET /api/trades?limit=100` | No | Yes | Enriched with parent order details. Remove fallback to `MOCK_TRADES`. |

---

### 2.8 Global Layout Components

| Component | Displayed Metric / Data | Current Source | Backend Endpoint | Uses Mock/Static Data? | Auto-Updates? | Required Correction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Topbar Ticker Strip** (`Topbar.jsx`) | Horizontal scrolling stock prices & 24h % | `stocks` state | `GET /api/stocks` | No | Yes | Live prices from backend. |
| **Topbar Connection Badge** (`Topbar.jsx`) | Connection State: `C++ Core Live` / `Standalone Mode` / `Backend Offline` | `connectionStatus` | `GET /api/health` | No | Yes | Clearly reflects true live backend connectivity. |
| **Topbar Trader Chip** (`Topbar.jsx`) | Current trader initial and first name | `currentTrader` | `GET /api/traders` | Partial (defaulted to `MOCK_TRADERS[0]`) | Yes | Initialize from `GET /api/traders`. |
| **Sidebar Navigation Badges** (`Sidebar.jsx`) | Open alerts count, Simulation `LIVE` pulse | `alerts`, `simulation` | `GET /api/alerts`, `GET /api/simulation/status` | No | Yes | Dynamically counts open alerts and simulation run state. |

---

## 3. Discovered Gaps & Action Plan

1. **`reviewAlert` parameter bug in `Shield.jsx`**:
   - `handleReview(alertId)` passed a string comment instead of `"REVIEWED"`, failing backend validation.
   - **Fix:** Update to pass `"REVIEWED"` and update state directly.
2. **Historical Stock Chart Data Gap**:
   - Backend `GET /api/stocks` did not return a `history` array, forcing React to synthesize intraday points in `normalizeStocks`.
   - **Fix:** Add a dedicated backend historical data provider or endpoint (`GET /api/stocks/{id}/history` and embedded `history` in `stockToJson`) that uses real SQLite trade executions merged with the baseline curve.
3. **Portfolio Performance Chart Gap**:
   - `PortfolioChart.jsx` used `MOCK_PORTFOLIO.performance_history` because the backend only provided current valuation.
   - **Fix:** Provide historical valuation points in `GET /api/portfolio` based on starting cash and trade executions over time.
4. **Mock Fallback Removal in Live Mode**:
   - In `api.js` and `TradeShieldProvider.jsx`, failing requests defaulted `useFallbackIfDisconnected` to true, returning `MOCK_*` constants.
   - **Fix:** Set live mode to never silently substitute mock data; set initial states to empty structures (`[]`, `null`) while loading, and surface disconnected error state when backend is unreachable.
5. **Dashboard Hero Header Hardcoding**:
   - `Dashboard.jsx` had static `"Last update: 2 min ago"`.
   - **Fix:** Bind to live timestamp from `last_tick_time` or latest API poll.
6. **Shield Trader Leaderboard Fallback**:
   - `Shield.jsx` contained `fallbackTraders` array.
   - **Fix:** Remove in LIVE mode and render `riskProfiles` from `GET /api/risk`.
