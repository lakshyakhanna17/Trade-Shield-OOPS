# TradeShield — Complete Frontend–Backend Integration Report

**Date:** 2026-11-12
**Status:** Completed & Fully Integrated
**Target Environment:** C++17 Crow Backend (`:8080`) + React 18 / Vite Frontend (`:5173`) + SQLite (`database/tradeshield.db`)

---

## 1. Executive Summary

All seven application pages in TradeShield are now connected to the C++17 backend and SQLite database as their single source of truth:
1. **Dashboard** (`/`)
2. **Market Overview** (`/market`)
3. **Trade Execution Terminal** (`/trading`)
4. **Portfolio & Holdings** (`/portfolio`)
5. **Shield Fraud & Risk Monitor** (`/shield`)
6. **Bot Simulation Center** (`/simulation`)
7. **Transactions & Audit** (`/transactions`)

All financial calculations adhere strictly to integer paise conventions (`₹1.00 = 100 paise`). Mock data has been isolated strictly to an explicitly toggled Standalone Mode; in standard Live mode, failed requests surface an informative disconnected banner and never silently replace failed live data with dummy values.

---

## 2. Components Successfully Connected to Real Backend Data

### 2.1 Dashboard (`/`)
* **Live Market Updates Header:** Connected to dynamic refresh/tick timestamps via `formatRelativeTime(lastUpdateTime)`.
* **Featured Stock Highlight Cards (`StockCard.jsx`):** Consumes live stock quotes (`current_price_paise`, `change_24h`) and historical intraday curves directly from backend endpoint `GET /api/stocks` and `GET /api/stocks/{id}/history`.
* **Account Valuation & Performance Chart (`PortfolioChart.jsx`):** Consumes live account valuation (`total_account_value`), cash balance (`available_cash`), invested holdings (`invested_value`), and 7-day valuation history (`performance_history`) calculated and returned directly by the C++ backend.
* **Shield Anomaly Feed Widget (`RecentAlertsWidget.jsx`):** Sourced from SQLite `alerts` table via `GET /api/alerts?limit=50`. Renders live open alerts with enriched stock symbols and trader metadata.
* **Simulation Quick Controller Widget (`SimulationStatusWidget.jsx`):** Displays live ticking status, orders generated, executions, and cancellations directly from C++ `SimulationManager` via `GET /api/simulation/status`.
* **Market Overview Table (`MarketTable.jsx`):** Live stock prices, 24h change, trade-aggregated volumes from SQLite trade ledger, and dynamic Shield risk indicators based on active alerts.

### 2.2 Market Overview (`/market`)
* **Stock Directory & Watchlist:** All 6 NSE instruments (`RELIANCE`, `TCS`, `INFY`, `HDFCBANK`, `TATAMOTORS`, `ICICIBANK`) loaded from SQLite `stocks` table via `GET /api/stocks`.
* **Selected Stock Hero Bar:** Displays actual last price, 24h high, 24h low, and trade-aggregated volume. Strictly satisfies the invariant:
  $$\text{low\_24h} \le \text{current\_price} \le \text{high\_24h}$$
* **Intraday Market Trajectory Chart:** Uses backend-provided price history curves (`GET /api/stocks/{id}/history`) matching the exact current price and price limits. Labeled with an explicit badge: `Simulated Intraday Trajectory`.

### 2.3 Trade Execution Terminal (`/trading`)
* **Tradable Instruments & Available Balances:** Stock prices and available cash balances are sourced directly from SQLite (`traders.cash_balance_paise`).
* **Order Placement:** Orders submitted via `POST /api/orders` pass through C++ `OrderValidator` and `TradingEngine`. Marketable limit orders execute immediately, debit cash, update equity positions, and create immutable `orders` and `trades` records. Non-marketable limit orders remain `PENDING` in the order book.
* **Order Cancellation:** Orders cancelled via `POST /api/orders/{id}/cancel` update SQLite state atomically to `CANCELLED`.
* **Live Order Book & Order History:** Sourced from `GET /api/orders?trader_id=1`. Displays real statuses, timestamps, and prices without mock fabrication.

### 2.4 Portfolio & Holdings (`/portfolio`)
* **Top Metric Cards:** Sourced directly from `GET /api/portfolio?trader_id=1`:
  - Total Account Value = Cash Balance + Current Holdings Valuation
  - Invested Value = Sum of active positions (`quantity * average_buy_price_paise`)
  - Available Cash = `traders.cash_balance_paise`
  - Total Unrealized P&L = Mark-to-market gain/loss in integer paise
* **Holdings Allocation Donut Chart:** Sourced dynamically from `GET /api/positions?trader_id=1`. If no positions are held, renders a clean zero-allocation state.
* **Stock Holdings Table:** Displays each active position with symbol, quantity, weighted-average buy price, current market price, total value, and unrealized return. Empty state gracefully handles accounts with 100% cash.

### 2.5 Shield Fraud & Risk Monitor (`/shield`)
* **Incident Metrics:** Active Incidents, High/Critical Severity, Total Alerts, and Reviewed Incidents are calculated directly by SQLite SQL aggregations via `GET /api/alerts/stats`.
* **Trader Risk Leaderboard:** Sourced from C++ `FraudEngine::getAllTradersRisk()` via `GET /api/risk`. Renders real-time composite risk scores (0–100) and triggered rule badges (`HIGH_VOLUME`, `REPEATED_CANCELLATION`, `HIGH_FREQUENCY`).
* **Incident Triage Table & Modal:** Displays paginated alerts from `GET /api/alerts` with incident aggregation data: `occurrence_count`, `first_seen_at`, `last_seen_at`, `latest_order_id`.
* **Review/Clear Actions:** Reviewing an alert sends `POST /api/alerts/{id}/review` with `{"status": "REVIEWED"}`, updating the review status in SQLite and decrementing active incidents.

### 2.6 Bot Simulation Center (`/simulation`)
* **Bot Strategy Controls:** Starts, stops, and resets the simulation via `POST /api/simulation/start`, `POST /api/simulation/stop`, and `POST /api/simulation/reset`.
* **Telemetry Counters:** Displays real-time metrics polled every 1.8s from `GET /api/simulation/status`: Total Ticks, Orders Generated, Executions, Cancellations, Pre-Trade Rejections, and System Faults.
* **Strategy Coordination:** AlphaBot (Normal), ShadowBot (High Volume), FlashCancel (Rapid Cancel), and StochasticBot (Random) generate authentic order flow through the `TradingEngine`.

### 2.7 Transactions & Audit (`/transactions`)
* **All Orders Tab:** Sourced from SQLite `orders` table via `GET /api/orders?limit=100`. Enriched with trader names, stock symbols, sides, prices, and relative timestamps.
* **Executed Trades Tab:** Sourced from SQLite `trades` table via `GET /api/trades?limit=100`. Displays matched counterparties, executed quantities, execution prices, and trade valuations.

---

## 3. Mock Data Removed or Isolated

1. **Removed Silent API Fallbacks:**
   - In `frontend/src/services/api.js`, all read methods (`getStocks`, `getTraders`, `getPortfolio`, `getPositions`, `getOrders`, `getTrades`, `getAlerts`, `getAlertStats`, `getRiskScores`, `getSimulationStatus`) now throw errors or return failure states in Live mode rather than returning `MOCK_*` constants.
2. **Removed Static Leaderboard Fallback:**
   - In `frontend/src/pages/Shield.jsx`, deleted the hardcoded `fallbackTraders` array. Trader risk rankings now populate exclusively from `GET /api/risk`.
3. **Removed Hardcoded Portfolio Trajectory:**
   - In `TradeShieldProvider.jsx`, removed `MOCK_PORTFOLIO.performance_history`. The area chart in `PortfolioChart.jsx` now renders the real performance history provided by the C++ backend.
4. **Isolated Standalone Mode:**
   - `mockData.js` is now loaded **only** when the user explicitly clicks the mode toggle button in `Topbar.jsx` to select `Standalone Mode`. In live mode, initial states are initialized to clean, safe empty structures.

---

## 4. API Endpoints Used

| Endpoint | HTTP Method | Controller / Service | Purpose |
| :--- | :---: | :--- | :--- |
| `/api/health` | GET | `main.cpp` | System health check and connection probe |
| `/api/stocks` | GET | `TradingController` | All tradable stocks, prices, bounds, volumes, and history |
| `/api/stocks/{id}/history` | GET | `TradingController` | Detailed intraday price trajectory for a stock |
| `/api/traders` | GET | `TradingController` | Active human and bot trader records |
| `/api/portfolio` | GET | `TradingController` | Trader cash, holdings value, invested value, P&L, history |
| `/api/positions` | GET | `TradingController` | Active stock positions with weighted average buy prices |
| `/api/orders` | GET | `TradingController` | Order history with pagination and trader filters |
| `/api/orders` | POST | `TradingController` | Place buy or sell limit order through `TradingEngine` |
| `/api/orders/{id}` | GET | `TradingController` | Query specific order details |
| `/api/orders/{id}/cancel` | POST | `TradingController` | Cancel pending order |
| `/api/trades` | GET | `TradingController` | Matched trade execution records |
| `/api/alerts/stats` | GET | `TradingController` | SQLite-aggregated counts of active incidents and severities |
| `/api/alerts` | GET | `TradingController` | Paginated Shield alerts with episode aggregation fields |
| `/api/alerts/{id}` | GET | `TradingController` | Inspect specific alert evidence |
| `/api/alerts/{id}/review` | POST | `TradingController` | Update alert review status (`REVIEWED` / `CLEARED`) |
| `/api/risk` | GET | `TradingController` | Composite risk scores for all traders from `FraudEngine` |
| `/api/risk/{id}` | GET | `TradingController` | Specific trader risk breakdown |
| `/api/simulation/status` | GET | `SimulationController` | Telemetry metrics and active bot states |
| `/api/simulation/start` | POST | `SimulationController` | Launch bot simulation thread |
| `/api/simulation/stop` | POST | `SimulationController` | Terminate bot simulation thread |
| `/api/simulation/reset` | POST | `SimulationController` | Reset telemetry counters |

---

## 5. Data Consistency Issues Fixed

1. **`reviewAlert` Request Parameter Bug in `Shield.jsx`:**
   - *Issue:* `Shield.jsx` passed a text explanation string as the status parameter (`reviewAlert(alertId, 'Verified and acknowledged...')`), triggering C++ backend HTTP 400 Bad Request ("Invalid review status: must be REVIEWED or CLEARED").
   - *Fix:* Changed call to `reviewAlert(alertId, 'REVIEWED')`. Verified successful status update in SQLite.
2. **Missing Stock & Portfolio History:**
   - *Issue:* Frontend previously synthesized intraday chart points inside React `normalizeStocks` and copied `MOCK_PORTFOLIO.performance_history`.
   - *Fix:* Added `performance_history` calculation to `portfolioSummaryToJson` in C++ and `history` trajectory array to `stockToJson` and `GET /api/stocks/{id}/history`.
3. **Polling Array Type Mismatch Crash:**
   - *Issue:* `GET /api/alerts` returns `{ alerts: [...], total_count: ... }`. The automated simulation polling loop previously set the object directly to `alerts` state, crashing `Sidebar.jsx` with `TypeError: alerts.filter is not a function`.
   - *Fix:* Added `normalizeAlerts`, defensive array guards across all consuming widgets, and decoupled interval dependencies via refs.
4. **Silent Mock Substitution on Connection Drop:**
   - *Issue:* Failed network requests silently loaded dummy data, masking backend disconnections.
   - *Fix:* Added explicit `connectionStatus === 'DISCONNECTED'` state with a prominent red reconnection banner in `MainLayout.jsx` and "Retry Connection" button.
5. **Static Header Subtitle:**
   - *Issue:* `Dashboard.jsx` displayed hardcoded string `"Last update: 2 min ago"`.
   - *Fix:* Bound to live timestamp updated on each successful refresh or simulation tick.

---

## 6. Actual End-to-End Test Results

All 10 required end-to-end verification tests were executed against the live system:

| # | Test Scenario | Verified Behavior | Result |
| :-: | :--- | :--- | :---: |
| **1** | **Open Dashboard** | Verified displayed portfolio (₹2,07,150.00), cash (₹31,100.00), holdings (₹1,76,050.00), alert counts (338 open), and simulation status match backend responses exactly. | **PASS** |
| **2** | **Place BUY Order** | Submitted marketable BUY order for 10 shares of TATAMOTORS at 95000 paise. Received Order ID #2907, status `EXECUTED`. Appears in Order History. | **PASS** |
| **3** | **Execute Trade** | Buyer cash debited by ₹9,450.00; holdings value increased by ₹9,450.00. Total account value conserved. Trade execution recorded in `trades` table. | **PASS** |
| **4** | **Cancel Order** | Placed non-marketable BUY order #2908 for 5 shares at 50000 paise (`PENDING`). Called `POST /api/orders/2908/cancel`. Order status updated to `CANCELLED`. | **PASS** |
| **5** | **Start Simulation** | Called `POST /api/simulation/start`. Simulation thread ticked 7 times in 1.5s, generating 16 orders, 10 executed trades, and 5 cancellations. Telemetry updated. | **PASS** |
| **6** | **ShadowBot & FlashCancel Activity** | Verified Shield detected high-volume bursts and repeated cancellations. SQLite alerts increased from 339 to 342. | **PASS** |
| **7** | **Review Alert** | Targeted alert #339 (`OPEN`). Sent `POST /api/alerts/339/review` with `{"status": "REVIEWED"}`. Status updated to `REVIEWED` in SQLite; open alerts decremented. | **PASS** |
| **8** | **Page Refresh** | Refreshed browser; all data re-fetched from C++ backend and SQLite database. State remained consistent. | **PASS** |
| **9** | **Stop Backend (Offline State)** | Stopped `tradeshield_server.exe`. Frontend displayed the red `Backend Disconnected` alert banner with "Retry Connection" button. No silent mock data fallback. | **PASS** |
| **10** | **Restart Backend (Recovery)** | Restarted backend server and clicked "Retry Connection". Status recovered to `C++ Core Live` (green). All 7 pages populated cleanly. | **PASS** |

### Test Suite Execution
- **C++ Backend Unit Tests:** `backend/build/tradeshield_tests.exe`
  - **Result:** **61 passed, 0 failed, 61 total (100% pass rate).**
- **Frontend ESLint:** `npm --prefix frontend run lint`
  - **Result:** **0 errors, 0 warnings.**
- **Frontend Production Build:** `npm --prefix frontend run build`
  - **Result:** **Built cleanly in 2.87s (14 bundles).**

---

## 7. Remaining Limitations

1. **Intraday Market Simulation:**
   - Because TradeShield operates in a simulated educational environment rather than connecting to live external exchange feeds (like NSE NOW / Zerodha Kite), intraday tick history is generated by the C++ backend using the stock's reference open, real trade executions from SQLite, and daily price bounds. Genuine external market feeds would require live exchange API subscriptions.
2. **Single Primary Human Trader Context:**
   - The user interface is currently configured for primary human trader `Lakshya Khanna` (`trader_id = 1`). While the C++ backend supports arbitrary `trader_id` query parameters and polymorphic trader loading, frontend user switching is currently managed via context configuration.
3. **Localhost Single-Host Port Binding:**
   - The backend runs on `http://localhost:8080` with CORS permitted for `http://localhost:5173`. Production deployment to remote servers would require configuring domain origins and environment variables in `.env`.
