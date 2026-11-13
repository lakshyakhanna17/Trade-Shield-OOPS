-- TradeShield SQLite Database Seed Data
-- Provides realistic instruments, traders, holdings, and initial baseline records

-- STOCKS (NSE Instruments)
INSERT INTO stocks (stock_id, symbol, company_name, exchange, current_price_paise, updated_at) VALUES
(1, 'RELIANCE', 'Reliance Industries Ltd', 'NSE', 295000, '2026-11-04T09:15:00Z'),
(2, 'TCS', 'Tata Consultancy Services Ltd', 'NSE', 385050, '2026-11-04T09:15:00Z'),
(3, 'INFY', 'Infosys Ltd', 'NSE', 152000, '2026-11-04T09:15:00Z'),
(4, 'HDFCBANK', 'HDFC Bank Ltd', 'NSE', 168000, '2026-11-04T09:15:00Z'),
(5, 'TATAMOTORS', 'Tata Motors Ltd', 'NSE', 94500, '2026-11-04T09:15:00Z'),
(6, 'ICICIBANK', 'ICICI Bank Ltd', 'NSE', 112000, '2026-11-04T09:15:00Z');

-- TRADERS
-- 1 Human trader and 4 Bot traders with distinct profiles
INSERT INTO traders (trader_id, name, trader_type, bot_strategy, cash_balance_paise, created_at) VALUES
(1, 'Lakshya (Human Trader)', 'HUMAN', NULL, 10000000, '2026-11-04T09:00:00Z'),
(2, 'AlphaBot (Normal Trader)', 'BOT', 'NORMAL', 50000000, '2026-11-04T09:00:00Z'),
(3, 'ShadowBot (High Volume)', 'BOT', 'HIGH_VOLUME', 100000000, '2026-11-04T09:00:00Z'),
(4, 'FlashCancel (Rapid Cancel)', 'BOT', 'RAPID_CANCEL', 50000000, '2026-11-04T09:00:00Z'),
(5, 'StochasticBot (Random)', 'BOT', 'RANDOM', 25000000, '2026-11-04T09:00:00Z');

-- POSITIONS (Initial holdings for testing sell orders and portfolio valuation)
-- Trader 1 (Human) holdings:
INSERT INTO positions (trader_id, stock_id, quantity, average_buy_price_paise) VALUES
(1, 1, 10, 290000),   -- 10 RELIANCE @ Rs 2,900.00
(1, 3, 20, 150000),   -- 20 INFY @ Rs 1,500.00
(1, 5, 50, 92000);    -- 50 TATAMOTORS @ Rs 920.00

-- Trader 2 (AlphaBot) holdings:
INSERT INTO positions (trader_id, stock_id, quantity, average_buy_price_paise) VALUES
(2, 2, 15, 380000),   -- 15 TCS @ Rs 3,800.00
(2, 4, 30, 165000);   -- 30 HDFCBANK @ Rs 1,650.00

-- Trader 3 (ShadowBot) holdings:
INSERT INTO positions (trader_id, stock_id, quantity, average_buy_price_paise) VALUES
(3, 3, 200, 151000),  -- 200 INFY @ Rs 1,510.00
(3, 5, 100, 93000);   -- 100 TATAMOTORS @ Rs 930.00

-- Baseline ORDERS and TRADES to demonstrate history on first launch
INSERT INTO orders (order_id, trader_id, stock_id, side, quantity, limit_price_paise, status, created_at, updated_at) VALUES
(1, 1, 1, 'BUY', 10, 290000, 'EXECUTED', '2026-11-04T09:16:00Z', '2026-11-04T09:16:01Z'),
(2, 1, 3, 'BUY', 20, 150000, 'EXECUTED', '2026-11-04T09:18:00Z', '2026-11-04T09:18:02Z'),
(3, 2, 2, 'BUY', 15, 380000, 'EXECUTED', '2026-11-04T09:20:00Z', '2026-11-04T09:20:01Z'),
(4, 3, 3, 'BUY', 200, 151000, 'EXECUTED', '2026-11-04T09:25:00Z', '2026-11-04T09:25:02Z'),
(5, 4, 1, 'BUY', 100, 294000, 'CANCELLED', '2026-11-04T09:30:00Z', '2026-11-04T09:30:05Z');

INSERT INTO trades (trade_id, order_id, executed_quantity, executed_price_paise, executed_at) VALUES
(1, 1, 10, 290000, '2026-11-04T09:16:01Z'),
(2, 2, 20, 150000, '2026-11-04T09:18:02Z'),
(3, 3, 15, 380000, '2026-11-04T09:20:01Z'),
(4, 4, 200, 151000, '2026-11-04T09:25:02Z');

-- Initial demo ALERTS (Illustrating Shield alert records)
INSERT INTO alerts (alert_id, trader_id, order_id, rule_name, severity, risk_points, description, created_at, review_status) VALUES
(1, 4, 5, 'REPEATED_CANCELLATION', 'MEDIUM', 45, 'Order #5 was cancelled within 5 seconds of placement (rapid cancellation pattern).', '2026-11-04T09:30:05Z', 'OPEN');

-- PORTFOLIOS summary cache
INSERT INTO portfolios (trader_id, total_value_paise, updated_at) VALUES
(1, 10000000 + (10 * 295000) + (20 * 152000) + (50 * 94500), '2026-11-04T09:35:00Z'),
(2, 50000000 + (15 * 385050) + (30 * 168000), '2026-11-04T09:35:00Z'),
(3, 100000000 + (200 * 152000) + (100 * 94500), '2026-11-04T09:35:00Z'),
(4, 50000000, '2026-11-04T09:35:00Z'),
(5, 25000000, '2026-11-04T09:35:00Z');
