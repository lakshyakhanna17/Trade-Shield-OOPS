# TradeShield — API Endpoints Specification

Base URL: `http://localhost:8080/api`

---

## 1. Stocks Endpoints

### `GET /api/stocks`
- **Description**: Returns all tradeable instruments with their latest prices.
- **Response**: `200 OK`
```json
[
  {
    "stock_id": 1,
    "symbol": "RELIANCE",
    "company_name": "Reliance Industries Ltd",
    "exchange": "NSE",
    "current_price_paise": 295000,
    "current_price_inr": 2950.00,
    "updated_at": "2026-11-04T09:15:00Z"
  }
]
```

### `GET /api/stocks/:symbol`
- **Description**: Returns detailed information for a specific stock ticker.
- **Response**: `200 OK`
```json
{
  "stock_id": 1,
  "symbol": "RELIANCE",
  "company_name": "Reliance Industries Ltd",
  "exchange": "NSE",
  "current_price_paise": 295000,
  "current_price_inr": 2950.00,
  "updated_at": "2026-11-04T09:15:00Z"
}
```

---

## 2. Traders & Portfolio Endpoints

### `GET /api/traders`
- **Description**: List all traders (human and automated bots).
- **Response**: `200 OK`
```json
[
  {
    "trader_id": 1,
    "name": "Lakshya (Human Trader)",
    "trader_type": "HUMAN",
    "bot_strategy": null,
    "cash_balance_paise": 10000000,
    "cash_balance_inr": 100000.00,
    "created_at": "2026-11-04T09:00:00Z"
  }
]
```

### `GET /api/portfolio`
- **Query Parameters**:
  - `trader_id` (optional, default: `1`)
- **Description**: Retrieves cash balance, active stock positions, and total portfolio valuation.
- **Response**: `200 OK`
```json
{
  "trader_id": 1,
  "trader_name": "Lakshya (Human Trader)",
  "cash_balance_paise": 10000000,
  "cash_balance_inr": 100000.00,
  "holdings_value_paise": 10675000,
  "holdings_value_inr": 106750.00,
  "total_value_paise": 20675000,
  "total_value_inr": 206750.00,
  "positions": [
    {
      "position_id": 1,
      "stock_id": 1,
      "symbol": "RELIANCE",
      "company_name": "Reliance Industries Ltd",
      "quantity": 10,
      "average_buy_price_paise": 290000,
      "current_price_paise": 295000,
      "total_invested_paise": 2900000,
      "current_value_paise": 2950000,
      "unrealized_pnl_paise": 50000,
      "unrealized_pnl_percentage": 1.72
    }
  ]
}
```

---

## 3. Order Management Endpoints

### `GET /api/orders`
- **Query Parameters**:
  - `trader_id` (optional)
  - `stock_id` (optional)
  - `status` (optional: `PENDING`, `EXECUTED`, `CANCELLED`, `REJECTED`)
  - `limit` (optional, default: `50`)
- **Description**: Fetches order history with trader, stock, and execution details.
- **Response**: `200 OK`
```json
[
  {
    "order_id": 1,
    "trader_id": 1,
    "trader_name": "Lakshya (Human Trader)",
    "stock_id": 1,
    "symbol": "RELIANCE",
    "side": "BUY",
    "quantity": 10,
    "limit_price_paise": 290000,
    "status": "EXECUTED",
    "created_at": "2026-11-04T09:16:00Z",
    "updated_at": "2026-11-04T09:16:01Z",
    "trades": [
      {
        "trade_id": 1,
        "executed_quantity": 10,
        "executed_price_paise": 290000,
        "executed_at": "2026-11-04T09:16:01Z"
      }
    ]
  }
]
```

### `POST /api/orders`
- **Description**: Submits a new buy or sell order for validation, fraud evaluation, and execution.
- **Request Body**:
```json
{
  "trader_id": 1,
  "stock_id": 3,
  "side": "BUY",
  "quantity": 15,
  "limit_price_paise": 152000
}
```
- **Response**: `201 Created`
```json
{
  "success": true,
  "message": "Order processed successfully",
  "order": {
    "order_id": 6,
    "status": "EXECUTED",
    "side": "BUY",
    "quantity": 15,
    "executed_price_paise": 152000
  },
  "alerts_triggered": []
}
```
- **Error Response**: `400 Bad Request`
```json
{
  "success": false,
  "error": "Insufficient cash balance. Required: 2280000 paise, Available: 1000000 paise"
}
```

### `POST /api/orders/:id/cancel`
- **Description**: Cancels an open/pending order and submits the event to the Shield anomaly detection engine.
- **Response**: `200 OK`
```json
{
  "success": true,
  "order_id": 5,
  "status": "CANCELLED",
  "alerts_triggered": [
    {
      "rule_name": "REPEATED_CANCELLATION",
      "severity": "MEDIUM",
      "risk_points": 45,
      "description": "Order cancelled within 3 seconds of placement."
    }
  ]
}
```

---

## 4. Shield Fraud Detection & Risk Endpoints

### `GET /api/alerts`
- **Query Parameters**:
  - `trader_id` (optional)
  - `severity` (optional: `LOW`, `MEDIUM`, `HIGH`)
  - `review_status` (optional: `OPEN`, `REVIEWED`, `CLEARED`)
  - `limit` (optional, default: `50`)
- **Description**: Lists detected fraud and anomaly alerts.
- **Response**: `200 OK`
```json
[
  {
    "alert_id": 1,
    "trader_id": 4,
    "trader_name": "FlashCancel (Rapid Cancel)",
    "order_id": 5,
    "rule_name": "REPEATED_CANCELLATION",
    "severity": "MEDIUM",
    "risk_points": 45,
    "description": "Order #5 was cancelled within 5 seconds of placement (rapid cancellation pattern).",
    "created_at": "2026-11-04T09:30:05Z",
    "review_status": "OPEN"
  }
]
```

### `POST /api/alerts/:id/review`
- **Description**: Updates compliance review status of an alert.
- **Request Body**:
```json
{
  "review_status": "REVIEWED"
}
```
- **Response**: `200 OK`
```json
{
  "success": true,
  "alert_id": 1,
  "review_status": "REVIEWED"
}
```

### `GET /api/risk/:traderId`
- **Description**: Computes composite risk profile, risk score (0–100), and categorical breakdown for a trader.
- **Response**: `200 OK`
```json
{
  "trader_id": 4,
  "trader_name": "FlashCancel (Rapid Cancel)",
  "trader_type": "BOT",
  "overall_risk_score": 68,
  "risk_tier": "MEDIUM",
  "total_alerts": 3,
  "breakdown": {
    "volume_risk": 15,
    "cancellation_risk": 50,
    "frequency_risk": 20
  },
  "recent_alerts": [
    {
      "rule_name": "REPEATED_CANCELLATION",
      "risk_points": 45,
      "created_at": "2026-11-04T09:30:05Z"
    }
  ]
}
```

---

## 5. Simulation Control Endpoints

### `GET /api/simulation/status`
- **Description**: Returns the real-time execution metrics and active bot identities of the simulation engine.
- **Response**: `200 OK`
```json
{
  "is_running": true,
  "tick_interval_ms": 1000,
  "total_ticks": 12,
  "total_orders_generated": 36,
  "total_executions_generated": 20,
  "total_cancellations_generated": 6,
  "total_errors": 0,
  "last_tick_time": "2026-11-11T06:19:55Z",
  "active_bots": [
    {"trader_id": 2, "name": "AlphaBot (Normal Trader)", "strategy": "NORMAL"},
    {"trader_id": 3, "name": "ShadowBot (High Volume)", "strategy": "HIGH_VOLUME"},
    {"trader_id": 4, "name": "FlashCancel (Rapid Cancel)", "strategy": "RAPID_CANCEL"},
    {"trader_id": 5, "name": "StochasticBot (Random)", "strategy": "RANDOM"}
  ]
}
```

### `POST /api/simulation/start`
- **Description**: Starts the background simulation thread. Safe and idempotent (rejects repeated starts with 400).
- **Request Body** (optional):
```json
{
  "tick_interval_ms": 1000,
  "bot_ids": [2, 3, 4, 5]
}
```
- **Response**: `200 OK`
```json
{
  "success": true,
  "message": "Simulation started successfully",
  "is_running": true,
  "tick_interval_ms": 1000,
  "active_bots_count": 4
}
```

### `POST /api/simulation/stop`
- **Description**: Gracefully halts the background simulation loop and joins the worker thread.
- **Response**: `200 OK`
```json
{
  "success": true,
  "message": "Simulation stopped successfully",
  "is_running": false,
  "total_orders_generated": 36,
  "total_executions_generated": 20,
  "total_cancellations_generated": 6
}
```

### `POST /api/simulation/reset`
- **Description**: Safely resets in-memory simulation performance counters without wiping database records.
- **Response**: `200 OK`
```json
{
  "success": true,
  "message": "Simulation performance metrics reset successfully. Database records preserved."
}
```

### `POST /api/simulation/tick`
- **Description**: Manually advances a single synchronous simulation tick across all active bots (useful for testing and debugging).
- **Response**: `200 OK`
```json
{
  "success": true,
  "actions_executed": 4,
  "results": [
    {"action_type": "BUY_ORDER", "order_id": 40, "message": "Buy order executed immediately at 295000 paise", "success": true},
    {"action_type": "BUY_ORDER", "order_id": 41, "message": "Buy order executed immediately at 152000 paise", "success": true},
    {"action_type": "CANCEL_ORDER", "order_id": 38, "message": "Order #38 cancelled successfully", "success": true},
    {"action_type": "BUY_ORDER", "order_id": 42, "message": "Buy order placed and pending execution (limit price below market)", "success": true}
  ]
}
```
