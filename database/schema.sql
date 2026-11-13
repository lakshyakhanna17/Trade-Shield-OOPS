-- TradeShield SQLite Database Schema
-- Follows TradeShield Database Schema & Entity Relationship Design

PRAGMA foreign_keys = ON;

-- TRADERS: People and simulated bots that can place orders
CREATE TABLE IF NOT EXISTS traders (
    trader_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    trader_type TEXT NOT NULL CHECK (trader_type IN ('HUMAN', 'BOT')),
    bot_strategy TEXT CHECK (bot_strategy IS NULL OR bot_strategy IN ('NORMAL', 'HIGH_VOLUME', 'RAPID_CANCEL', 'RANDOM')),
    cash_balance_paise INTEGER NOT NULL DEFAULT 0 CHECK (cash_balance_paise >= 0),
    created_at TEXT NOT NULL
);

-- STOCKS: Instruments available in the simulator
CREATE TABLE IF NOT EXISTS stocks (
    stock_id INTEGER PRIMARY KEY AUTOINCREMENT,
    symbol TEXT NOT NULL UNIQUE,
    company_name TEXT NOT NULL,
    exchange TEXT NOT NULL,
    current_price_paise INTEGER NOT NULL CHECK (current_price_paise > 0),
    updated_at TEXT NOT NULL
);

-- ORDERS: All manual and bot buy/sell requests
CREATE TABLE IF NOT EXISTS orders (
    order_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE RESTRICT,
    stock_id INTEGER NOT NULL REFERENCES stocks(stock_id) ON DELETE RESTRICT,
    side TEXT NOT NULL CHECK (side IN ('BUY', 'SELL')),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    limit_price_paise INTEGER NOT NULL CHECK (limit_price_paise > 0),
    status TEXT NOT NULL CHECK (status IN ('PENDING', 'PARTIALLY_FILLED', 'EXECUTED', 'CANCELLED', 'REJECTED')),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

-- TRADES: Successful executions
CREATE TABLE IF NOT EXISTS trades (
    trade_id INTEGER PRIMARY KEY AUTOINCREMENT,
    order_id INTEGER NOT NULL REFERENCES orders(order_id) ON DELETE RESTRICT,
    executed_quantity INTEGER NOT NULL CHECK (executed_quantity > 0),
    executed_price_paise INTEGER NOT NULL CHECK (executed_price_paise > 0),
    executed_at TEXT NOT NULL
);

-- POSITIONS: Current holdings per trader and stock
CREATE TABLE IF NOT EXISTS positions (
    position_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,
    stock_id INTEGER NOT NULL REFERENCES stocks(stock_id) ON DELETE RESTRICT,
    quantity INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    average_buy_price_paise INTEGER NOT NULL DEFAULT 0 CHECK (average_buy_price_paise >= 0),
    UNIQUE(trader_id, stock_id)
);

-- ALERTS: Fraud/anomaly findings produced by Shield
CREATE TABLE IF NOT EXISTS alerts (
    alert_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,
    order_id INTEGER REFERENCES orders(order_id) ON DELETE SET NULL,
    rule_name TEXT NOT NULL,
    severity TEXT NOT NULL CHECK (severity IN ('LOW', 'MEDIUM', 'HIGH')),
    risk_points INTEGER NOT NULL CHECK (risk_points BETWEEN 0 AND 100),
    description TEXT NOT NULL,
    created_at TEXT NOT NULL,
    review_status TEXT NOT NULL DEFAULT 'OPEN' CHECK (review_status IN ('OPEN', 'REVIEWED', 'CLEARED')),
    first_seen_at TEXT,
    last_seen_at TEXT,
    occurrence_count INTEGER NOT NULL DEFAULT 1,
    latest_order_id INTEGER REFERENCES orders(order_id) ON DELETE SET NULL
);

-- PORTFOLIOS: Optional cached summary per trader
CREATE TABLE IF NOT EXISTS portfolios (
    portfolio_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL UNIQUE REFERENCES traders(trader_id) ON DELETE CASCADE,
    total_value_paise INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT NOT NULL
);

-- PORTFOLIO_SNAPSHOTS: Historical portfolio valuation records for performance charting
CREATE TABLE IF NOT EXISTS portfolio_snapshots (
    snapshot_id INTEGER PRIMARY KEY AUTOINCREMENT,
    trader_id INTEGER NOT NULL REFERENCES traders(trader_id) ON DELETE CASCADE,
    total_value_paise INTEGER NOT NULL CHECK (total_value_paise >= 0),
    cash_balance_paise INTEGER NOT NULL CHECK (cash_balance_paise >= 0),
    holdings_value_paise INTEGER NOT NULL CHECK (holdings_value_paise >= 0),
    snapshot_time TEXT NOT NULL
);

-- Indexes for performance and quick lookups
CREATE INDEX IF NOT EXISTS idx_orders_trader_created ON orders(trader_id, created_at);
CREATE INDEX IF NOT EXISTS idx_orders_stock_created ON orders(stock_id, created_at);
CREATE INDEX IF NOT EXISTS idx_orders_status_created ON orders(status, created_at);
CREATE INDEX IF NOT EXISTS idx_alerts_trader_created ON alerts(trader_id, created_at);
CREATE INDEX IF NOT EXISTS idx_alerts_trader_rule_status ON alerts(trader_id, rule_name, review_status);
CREATE INDEX IF NOT EXISTS idx_trades_order_id ON trades(order_id);
CREATE INDEX IF NOT EXISTS idx_positions_trader ON positions(trader_id);
CREATE INDEX IF NOT EXISTS idx_portfolio_snapshots_trader_time ON portfolio_snapshots(trader_id, snapshot_time);
