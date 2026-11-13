# TradeShield — Stage 8 Frontend–Backend Integration Mapping

This document provides the definitive contract mapping between the React frontend components, Axios API service methods, C++17 Crow REST endpoints, JSON schemas, affected SQLite database tables, and their integration status.

---

## 1. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    React Frontend (Vite)                    │
│            http://localhost:5173  (UI & State)              │
└──────────────────────────────┬──────────────────────────────┘
                               │ HTTP / JSON REST
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                Crow C++17 HTTP Backend API                  │
│                     http://localhost:8080                   │
├──────────────────────────────┬──────────────────────────────┤
│ TradingController            │ SimulationController         │
├──────────────────────────────┴──────────────────────────────┤
│ Core Engine Services                                        │
│  - TradingEngine (Order validation, execution, portfolio)    │
│  - FraudEngine (Pattern analysis, sliding windows, scoring) │
│  - SimulationManager (Thread coordinator, 4 bot strategies) │
├─────────────────────────────────────────────────────────────┤
│ SQLite Repositories (RAII DatabaseManager & Statement)      │
│  - traders, stocks, orders, trades, positions, portfolios,  │
│    alerts                                                   │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. API Contract Mapping Matrix

### 2.1 System Health
* **Frontend Component**: `Topbar.jsx`, `TradeShieldProvider.jsx`
* **API Service Method**: `api.getHealth()`
* **Backend Endpoint**: `GET /api/health`
* **Request Format**: None
* **Response Format**:
  ```json
  {
    "status": "healthy",
    "service": "TradeShield Backend API",
    "version": "1.0.0",
    "environment": "development"
  }
  ```
* **Database Tables**: None (In-memory heartbeat)
* **Integration Status**: Ready / Verified

---

### 2.2 Stocks & Market Data
* **Frontend Component**: `Market.jsx`, `StockCard.jsx`, `MarketTable.jsx`, `Trading.jsx`, `Topbar.jsx`
* **API Service Method**: `api.getStocks()`, `api.getStockById(id)`
* **Backend Endpoint**: `GET /api/stocks`
* **Request Format**: None
* **Response Format**:
  ```json
  [
    {
      "stock_id": 1,
      "symbol": "RELIANCE",
      "company_name": "Reliance Industries Ltd",
      "exchange": "NSE",
      "current_price_paise": 245000,
      "current_price_inr": 2450.0,
      "updated_at": "2026-11-04T09:30:00Z"
    }
  ]
  ```
* **Database Tables**: `stocks` (Read)
* **Integration Status**: Ready for live wire-up; visual sparkline history maintained client-side.

---

### 2.3 Trading & Order Management
#### A. Submit Order
* **Frontend Component**: `Trading.jsx` (Order Entry form & Confirmation Modal)
* **API Service Method**: `api.placeOrder(payload)`
* **Backend Endpoint**: `POST /api/orders`
* **Request Format**:
  ```json
  {
    "trader_id": 1,
    "stock_id": 1,
    "side": "BUY",
    "quantity": 10,
    "limit_price_paise": 245000
  }
  ```
* **Response Format**:
  ```json
  {
    "success": true,
    "message": "Marketable BUY order executed immediately at market price",
    "order": {
      "order_id": 101,
      "trader_id": 1,
      "stock_id": 1,
      "side": "BUY",
      "quantity": 10,
      "limit_price_paise": 245000,
      "limit_price_inr": 2450.0,
      "status": "EXECUTED",
      "created_at": "2026-11-12T13:10:00Z",
      "updated_at": "2026-11-12T13:10:00Z"
    },
    "trade": {
      "trade_id": 52,
      "order_id": 101,
      "executed_quantity": 10,
      "executed_price_paise": 245000,
      "executed_price_inr": 2450.0,
      "total_value_paise": 2450000,
      "total_value_inr": 24500.0,
      "executed_at": "2026-11-12T13:10:00Z"
    }
  }
  ```
* **Database Tables**: `orders`, `trades` (if executed), `positions` (if executed), `portfolios` (if executed), `traders` (cash balance).
* **Integration Status**: Ready. Requires strict error bubbling on validation rejection (e.g. insufficient cash).

#### B. Query Orders
* **Frontend Component**: `Transactions.jsx`, `Trading.jsx` (Recent Orders), `Dashboard.jsx`
* **API Service Method**: `api.getOrders(params)`
* **Backend Endpoint**: `GET /api/orders?limit=100&trader_id=1`
* **Response Format**: Array of order objects.
* **Database Tables**: `orders` (Read).
* **Integration Status**: Ready.

#### C. Cancel Order
* **Frontend Component**: `Trading.jsx`, `Transactions.jsx`
* **API Service Method**: `api.cancelOrder(orderId)`
* **Backend Endpoint**: `POST /api/orders/<int>/cancel`
* **Request Format**: None (ID in URL path)
* **Response Format**:
  ```json
  {
    "success": true,
    "message": "Order #101 cancelled successfully",
    "order": { ... "status": "CANCELLED" }
  }
  ```
* **Database Tables**: `orders` (Status update to `CANCELLED`).
* **Integration Status**: Ready.

---

### 2.4 Executed Trades History
* **Frontend Component**: `Transactions.jsx` (Trades tab), `Dashboard.jsx`
* **API Service Method**: `api.getTrades(params)`
* **Backend Endpoint**: `GET /api/trades?limit=100`
* **Response Format**: Array of trade objects.
* **Database Tables**: `trades` (Read).
* **Integration Status**: Ready.

---

### 2.5 Portfolio & Position Valuations
#### A. Portfolio Summary
* **Frontend Component**: `Portfolio.jsx`, `PortfolioChart.jsx`, `Dashboard.jsx`
* **API Service Method**: `api.getPortfolio(traderId)`
* **Backend Endpoint**: `GET /api/portfolio?trader_id=1`
* **Response Format**:
  ```json
  {
    "trader_id": 1,
    "trader_name": "Lakshya Khanna",
    "trader_type": "HUMAN",
    "cash_balance_paise": 50000000,
    "cash_balance_inr": 500000.0,
    "holdings_value_paise": 59010000,
    "holdings_value_inr": 590100.0,
    "total_invested_paise": 57940000,
    "total_invested_inr": 579400.0,
    "total_value_paise": 109010000,
    "total_value_inr": 1090100.0,
    "unrealized_pnl_paise": 1070000,
    "unrealized_pnl_inr": 10700.0,
    "unrealized_pnl_percentage": 1.8467,
    "positions": [ ... ]
  }
  ```
* **Database Tables**: `portfolios`, `traders`, `positions`, `stocks` (Read & compute).
* **Integration Status**: Ready.

#### B. Active Stock Positions
* **Frontend Component**: `Portfolio.jsx` (Holdings table)
* **API Service Method**: `api.getPositions(traderId)`
* **Backend Endpoint**: `GET /api/positions?trader_id=1`
* **Response Format**: Array of `PositionDetail` objects.
* **Database Tables**: `positions`, `stocks` (Read).
* **Integration Status**: Ready.

---

### 2.6 Shield Fraud & Anomaly Intelligence
#### A. List Alerts
* **Frontend Component**: `Shield.jsx`, `RecentAlertsWidget.jsx`, `Sidebar.jsx` (Badge count)
* **API Service Method**: `api.getAlerts(params)`
* **Backend Endpoint**: `GET /api/alerts?limit=100&severity=HIGH&review_status=OPEN`
* **Response Format**: Array of alert objects:
  ```json
  [
    {
      "alert_id": 33,
      "trader_id": 4,
      "rule_name": "REPEATED_CANCELLATION",
      "severity": "HIGH",
      "risk_points": 70,
      "description": "Rapid cancellation attack pattern: 5 cancellations in 60s (100.0% cancel ratio).",
      "created_at": "2026-11-12T13:10:00Z",
      "review_status": "OPEN",
      "order_id": 66
    }
  ]
  ```
* **Database Tables**: `alerts` (Read).
* **Integration Status**: Ready.

#### B. Review / Clear Alert
* **Frontend Component**: `Shield.jsx` (Clear button & Modal)
* **API Service Method**: `api.reviewAlert(alertId, status)`
* **Backend Endpoint**: `POST /api/alerts/<int>/review`
* **Request Format**:
  ```json
  {
    "status": "REVIEWED"
  }
  ```
* **Response Format**:
  ```json
  {
    "success": true,
    "message": "Alert status updated to REVIEWED",
    "alert": { ... "review_status": "REVIEWED" }
  }
  ```
* **Database Tables**: `alerts` (Update `review_status`).
* **Integration Status**: Ready.

#### C. Trader Risk Profiles
* **Frontend Component**: `Shield.jsx` (Trader Risk Leaderboard)
* **API Service Method**: `api.getRiskScores()`
* **Backend Endpoint**: `GET /api/risk`
* **Response Format**: Array of `TraderRiskProfile` objects:
  ```json
  [
    {
      "trader_id": 4,
      "trader_name": "FlashCancel (Rapid Cancel)",
      "overall_risk_score": 70,
      "risk_tier": "HIGH",
      "total_alerts": 2,
      "low_severity_alerts": 0,
      "medium_severity_alerts": 1,
      "high_severity_alerts": 1,
      "unreviewed_alerts": 2,
      "last_alert_time": "2026-11-12T13:10:00Z",
      "triggered_rules": ["REPEATED_CANCELLATION", "HIGH_FREQUENCY"]
    }
  ]
  ```
* **Database Tables**: `alerts`, `traders` (Computed via `FraudEngine::getAllTradersRisk`).
* **Integration Status**: Ready.

---

### 2.7 Bot Simulation Engine
#### A. Start Simulation
* **Frontend Component**: `Simulation.jsx`, `SimulationStatusWidget.jsx`
* **API Service Method**: `api.startSimulation(tickIntervalMs)`
* **Backend Endpoint**: `POST /api/simulation/start`
* **Request Format**:
  ```json
  {
    "tick_interval_ms": 2000
  }
  ```
* **Response Format**:
  ```json
  {
    "success": true,
    "message": "Simulation started successfully",
    "is_running": true,
    "tick_interval_ms": 2000,
    "active_bots_count": 4
  }
  ```
* **Database Tables**: Initiates background worker that inserts into `orders`, `trades`, `positions`, `alerts`.
* **Integration Status**: Ready.

#### B. Stop Simulation
* **Frontend Component**: `Simulation.jsx`, `SimulationStatusWidget.jsx`
* **API Service Method**: `api.stopSimulation()`
* **Backend Endpoint**: `POST /api/simulation/stop`
* **Response Format**:
  ```json
  {
    "success": true,
    "message": "Simulation stopped successfully",
    "is_running": false,
    "total_orders_generated": 34,
    "total_executions_generated": 24,
    "total_cancellations_generated": 6
  }
  ```
* **Integration Status**: Ready.

#### C. Get Simulation Status
* **Frontend Component**: `Simulation.jsx`, `SimulationStatusWidget.jsx`, `Topbar.jsx`
* **API Service Method**: `api.getSimulationStatus()`
* **Backend Endpoint**: `GET /api/simulation/status`
* **Response Format**:
  ```json
  {
    "is_running": false,
    "tick_interval_ms": 2000,
    "total_ticks": 10,
    "total_orders_generated": 34,
    "total_executions_generated": 24,
    "total_cancellations_generated": 6,
    "total_errors": 0,
    "last_tick_time": "2026-11-12T13:10:00Z",
    "active_bots": [
      { "trader_id": 2, "name": "AlphaBot (Normal Trader)", "strategy": "NORMAL" },
      { "trader_id": 3, "name": "ShadowBot (High Volume)", "strategy": "HIGH_VOLUME" },
      { "trader_id": 4, "name": "FlashCancel (Rapid Cancel)", "strategy": "RAPID_CANCEL" },
      { "trader_id": 5, "name": "StochasticBot (Random)", "strategy": "RANDOM" }
    ]
  }
  ```
* **Integration Status**: Ready.

#### D. Reset Simulation Metrics
* **Frontend Component**: `Simulation.jsx`, `SimulationStatusWidget.jsx`
* **API Service Method**: `api.resetSimulation()`
* **Backend Endpoint**: `POST /api/simulation/reset`
* **Integration Status**: Ready (Clears in-memory counters; preserves historical SQLite data).

---

## 3. Traders Querying
* **Frontend Component**: `TradeShieldProvider.jsx`
* **API Service Method**: `api.getTraders()`
* **Backend Endpoint**: `GET /api/traders`
* **Response Format**: Array of trader objects.
* **Database Tables**: `traders` (Read).
* **Integration Status**: Ready.
