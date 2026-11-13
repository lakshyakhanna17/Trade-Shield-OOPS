/**
 * TradeShield Realistic Mock Data
 * Mirrors SQLite database/seed.sql and backend domain models.
 * Used for standalone frontend development and fallback mode.
 */

export const MOCK_TRADERS = [
  {
    trader_id: 1,
    name: 'Lakshya Khanna',
    type: 'HUMAN',
    status: 'ACTIVE',
    cash_balance: 50000000, // ₹5,00,000.00 in paise
    risk_score: 0,
    created_at: '2026-11-01T09:00:00Z',
  },
  {
    trader_id: 2,
    name: 'AlphaBot (Normal Trader)',
    type: 'BOT',
    status: 'ACTIVE',
    cash_balance: 100000000, // ₹10,00,000.00
    risk_score: 0,
    strategy: 'NORMAL',
    created_at: '2026-11-01T09:00:00Z',
  },
  {
    trader_id: 3,
    name: 'ShadowBot (High Volume)',
    type: 'BOT',
    status: 'ACTIVE',
    cash_balance: 200000000, // ₹20,00,000.00
    risk_score: 65,
    strategy: 'HIGH_VOLUME',
    created_at: '2026-11-01T09:00:00Z',
  },
  {
    trader_id: 4,
    name: 'FlashCancel (Rapid Cancel)',
    type: 'BOT',
    status: 'ACTIVE',
    cash_balance: 100000000, // ₹10,00,000.00
    risk_score: 70,
    strategy: 'RAPID_CANCEL',
    created_at: '2026-11-01T09:00:00Z',
  },
  {
    trader_id: 5,
    name: 'StochasticBot (Random)',
    type: 'BOT',
    status: 'ACTIVE',
    cash_balance: 100000000, // ₹10,00,000.00
    risk_score: 15,
    strategy: 'RANDOM',
    created_at: '2026-11-01T09:00:00Z',
  },
];

export const MOCK_STOCKS = [
  {
    stock_id: 1,
    symbol: 'RELIANCE',
    name: 'Reliance Industries Ltd',
    current_price: 295000, // ₹2,950.00 in paise
    change_24h: 1.24,
    change_7d: 3.45,
    open_price: 291400,
    high_24h: 297500,
    low_24h: 289800,
    volume: 142580,
    accent_color: '#F59E0B', // Amber
    shield_risk: 'LOW',
    history: [
      { time: '09:15', price: 2914.0 },
      { time: '10:00', price: 2928.5 },
      { time: '11:00', price: 2942.0 },
      { time: '12:00', price: 2960.0 },
      { time: '13:00', price: 2938.0 },
      { time: '14:00', price: 2970.0 },
      { time: '15:30', price: 2950.0 },
    ],
  },
  {
    stock_id: 2,
    symbol: 'TCS',
    name: 'Tata Consultancy Services',
    current_price: 385050, // ₹3,850.50 in paise
    change_24h: -0.81,
    change_7d: 1.15,
    open_price: 388200,
    high_24h: 389500,
    low_24h: 383500,
    volume: 98400,
    accent_color: '#3B82F6', // Blue
    shield_risk: 'LOW',
    history: [
      { time: '09:15', price: 3882.0 },
      { time: '10:00', price: 3875.0 },
      { time: '11:00', price: 3862.0 },
      { time: '12:00', price: 3845.0 },
      { time: '13:00', price: 3860.0 },
      { time: '14:00', price: 3840.0 },
      { time: '15:30', price: 3850.5 },
    ],
  },
  {
    stock_id: 3,
    symbol: 'INFY',
    name: 'Infosys Limited',
    current_price: 152000, // ₹1,520.00 in paise
    change_24h: 0.53,
    change_7d: -1.20,
    open_price: 151200,
    high_24h: 153500,
    low_24h: 150000,
    volume: 185200,
    accent_color: '#8B5CF6', // Purple
    shield_risk: 'MEDIUM',
    history: [
      { time: '09:15', price: 1512.0 },
      { time: '10:00', price: 1518.0 },
      { time: '11:00', price: 1508.0 },
      { time: '12:00', price: 1525.0 },
      { time: '13:00', price: 1515.0 },
      { time: '14:00', price: 1530.0 },
      { time: '15:30', price: 1520.0 },
    ],
  },
  {
    stock_id: 4,
    symbol: 'HDFCBANK',
    name: 'HDFC Bank Limited',
    current_price: 168000, // ₹1,680.00 in paise
    change_24h: 1.08,
    change_7d: 2.80,
    open_price: 166200,
    high_24h: 169500,
    low_24h: 165000,
    volume: 210400,
    accent_color: '#10B981', // Emerald
    shield_risk: 'LOW',
    history: [
      { time: '09:15', price: 1662.0 },
      { time: '10:00', price: 1670.0 },
      { time: '11:00', price: 1658.0 },
      { time: '12:00', price: 1685.0 },
      { time: '13:00', price: 1672.0 },
      { time: '14:00', price: 1690.0 },
      { time: '15:30', price: 1680.0 },
    ],
  },
  {
    stock_id: 5,
    symbol: 'TATAMOTORS',
    name: 'Tata Motors Limited',
    current_price: 94500, // ₹945.00 in paise
    change_24h: 2.38,
    change_7d: 5.60,
    open_price: 92300,
    high_24h: 95500,
    low_24h: 91500,
    volume: 320100,
    accent_color: '#EC4899', // Pink
    shield_risk: 'LOW',
    history: [
      { time: '09:15', price: 923.0 },
      { time: '10:00', price: 930.0 },
      { time: '11:00', price: 920.0 },
      { time: '12:00', price: 940.0 },
      { time: '13:00', price: 935.0 },
      { time: '14:00', price: 952.0 },
      { time: '15:30', price: 945.0 },
    ],
  },
  {
    stock_id: 6,
    symbol: 'ICICIBANK',
    name: 'ICICI Bank Limited',
    current_price: 112000, // ₹1,120.00 in paise
    change_24h: 0.45,
    change_7d: 0.90,
    open_price: 111500,
    high_24h: 112800,
    low_24h: 110800,
    volume: 165000,
    accent_color: '#06B6D4', // Cyan
    shield_risk: 'LOW',
    history: [
      { time: '09:15', price: 1115.0 },
      { time: '10:00', price: 1118.5 },
      { time: '11:00', price: 1112.0 },
      { time: '12:00', price: 1122.0 },
      { time: '13:00', price: 1119.0 },
      { time: '14:00', price: 1126.0 },
      { time: '15:30', price: 1120.0 },
    ],
  },
];

export const MOCK_HOLDINGS = [
  {
    position_id: 1,
    trader_id: 1,
    stock_id: 1,
    symbol: 'RELIANCE',
    name: 'Reliance Industries Ltd',
    quantity: 100,
    average_buy_price: 238000, // ₹2,380.00
    invested_value: 23800000, // ₹2,38,000.00
    current_price: 245000,
    current_value: 24500000, // ₹2,45,000.00
    unrealized_pnl: 700000, // +₹7,000.00
    pnl_percent: 2.94,
    allocation_percent: 41.5,
  },
  {
    position_id: 2,
    trader_id: 1,
    stock_id: 2,
    symbol: 'TCS',
    name: 'Tata Consultancy Services',
    quantity: 50,
    average_buy_price: 345000, // ₹3,450.00
    invested_value: 17250000, // ₹1,72,500.00
    current_price: 352000,
    current_value: 17600000, // ₹1,76,000.00
    unrealized_pnl: 350000, // +₹3,500.00
    pnl_percent: 2.03,
    allocation_percent: 29.8,
  },
  {
    position_id: 3,
    trader_id: 1,
    stock_id: 3,
    symbol: 'INFY',
    name: 'Infosys Limited',
    quantity: 80,
    average_buy_price: 151000, // ₹1,510.00
    invested_value: 12080000, // ₹1,20,800.00
    current_price: 148000,
    current_value: 11840000, // ₹1,18,400.00
    unrealized_pnl: -240000, // -₹2,400.00
    pnl_percent: -1.99,
    allocation_percent: 20.1,
  },
  {
    position_id: 4,
    trader_id: 1,
    stock_id: 5,
    symbol: 'TATAMOTORS',
    name: 'Tata Motors Limited',
    quantity: 65,
    average_buy_price: 74000, // ₹740.00
    invested_value: 4810000, // ₹48,100.00
    current_price: 78000,
    current_value: 5070000, // ₹50,700.00
    unrealized_pnl: 260000, // +₹2,600.00
    pnl_percent: 5.41,
    allocation_percent: 8.6,
  },
];

export const MOCK_PORTFOLIO = {
  trader_id: 1,
  trader_name: 'Lakshya Khanna',
  available_cash: 50000000, // ₹5,00,000.00
  invested_value: 57940000, // ₹5,79,400.00
  current_portfolio_value: 59010000, // ₹5,90,100.00
  total_account_value: 109010000, // ₹10,90,100.00 (Cash + Portfolio)
  total_unrealized_pnl: 1070000, // +₹10,700.00
  total_pnl_percent: 1.85,
  daily_change: 845000, // +₹8,450.00
  daily_change_percent: 0.78,
  performance_history: [
    { date: '04 Nov', portfolioValue: 1050000, cash: 500000 },
    { date: '05 Nov', portfolioValue: 1058000, cash: 500000 },
    { date: '06 Nov', portfolioValue: 1064000, cash: 495000 },
    { date: '07 Nov', portfolioValue: 1072000, cash: 495000 },
    { date: '08 Nov', portfolioValue: 1070000, cash: 498000 },
    { date: '09 Nov', portfolioValue: 1081000, cash: 500000 },
    { date: '10 Nov', portfolioValue: 1085000, cash: 500000 },
    { date: '11 Nov', portfolioValue: 1090100, cash: 500000 },
  ],
};

export const MOCK_ORDERS = [
  {
    order_id: 101,
    trader_id: 1,
    trader_name: 'Lakshya Khanna',
    stock_id: 1,
    symbol: 'RELIANCE',
    side: 'BUY',
    type: 'LIMIT',
    quantity: 25,
    price: 244500, // ₹2,445.00
    status: 'PENDING',
    created_at: '2026-11-11T06:15:20Z',
  },
  {
    order_id: 100,
    trader_id: 1,
    trader_name: 'Lakshya Khanna',
    stock_id: 4,
    symbol: 'HDFCBANK',
    side: 'BUY',
    type: 'MARKET',
    quantity: 20,
    price: 165000,
    status: 'EXECUTED',
    created_at: '2026-11-11T05:45:00Z',
  },
  {
    order_id: 99,
    trader_id: 3,
    trader_name: 'ShadowBot (High Volume)',
    stock_id: 1,
    symbol: 'RELIANCE',
    side: 'BUY',
    type: 'LIMIT',
    quantity: 150,
    price: 245000,
    status: 'EXECUTED',
    created_at: '2026-11-11T06:26:41Z',
  },
  {
    order_id: 98,
    trader_id: 4,
    trader_name: 'FlashCancel (Rapid Cancel)',
    stock_id: 3,
    symbol: 'INFY',
    side: 'BUY',
    type: 'LIMIT',
    quantity: 50,
    price: 133200,
    status: 'CANCELLED',
    created_at: '2026-11-11T06:26:40Z',
  },
  {
    order_id: 97,
    trader_id: 2,
    trader_name: 'AlphaBot (Normal Trader)',
    stock_id: 2,
    symbol: 'TCS',
    side: 'BUY',
    type: 'LIMIT',
    quantity: 8,
    price: 352000,
    status: 'EXECUTED',
    created_at: '2026-11-11T06:25:00Z',
  },
  {
    order_id: 96,
    trader_id: 1,
    trader_name: 'Lakshya Khanna',
    stock_id: 5,
    symbol: 'TATAMOTORS',
    side: 'SELL',
    type: 'LIMIT',
    quantity: 15,
    price: 78500,
    status: 'EXECUTED',
    created_at: '2026-11-10T14:20:10Z',
  },
];

export const MOCK_TRADES = [
  {
    trade_id: 51,
    buy_order_id: 100,
    sell_order_id: 95,
    buyer_id: 1,
    buyer_name: 'Lakshya Khanna',
    seller_id: 2,
    seller_name: 'AlphaBot',
    stock_id: 4,
    symbol: 'HDFCBANK',
    quantity: 20,
    execution_price: 165000, // ₹1,650.00
    trade_value: 3300000, // ₹33,000.00
    executed_at: '2026-11-11T05:45:01Z',
  },
  {
    trade_id: 50,
    buy_order_id: 99,
    sell_order_id: 92,
    buyer_id: 3,
    buyer_name: 'ShadowBot',
    seller_id: 5,
    seller_name: 'StochasticBot',
    stock_id: 1,
    symbol: 'RELIANCE',
    quantity: 150,
    execution_price: 245000,
    trade_value: 36750000, // ₹3,67,500.00
    executed_at: '2026-11-11T06:26:41Z',
  },
  {
    trade_id: 49,
    buy_order_id: 97,
    sell_order_id: 88,
    buyer_id: 2,
    buyer_name: 'AlphaBot',
    seller_id: 1,
    seller_name: 'Lakshya Khanna',
    stock_id: 2,
    symbol: 'TCS',
    quantity: 8,
    execution_price: 352000,
    trade_value: 2816000, // ₹28,160.00
    executed_at: '2026-11-11T06:25:01Z',
  },
  {
    trade_id: 48,
    buy_order_id: 90,
    sell_order_id: 96,
    buyer_id: 2,
    buyer_name: 'AlphaBot',
    seller_id: 1,
    seller_name: 'Lakshya Khanna',
    stock_id: 5,
    symbol: 'TATAMOTORS',
    quantity: 15,
    execution_price: 78500,
    trade_value: 1177500, // ₹11,775.00
    executed_at: '2026-11-10T14:20:12Z',
  },
];

export const MOCK_ALERTS = [
  {
    alert_id: 33,
    trader_id: 4,
    trader_name: 'FlashCancel (Rapid Cancel)',
    order_id: 66,
    symbol: 'INFY',
    rule_name: 'REPEATED_CANCELLATION',
    severity: 'HIGH',
    risk_points: 70,
    description: 'Rapid cancellation attack pattern: 5 cancellations in 60s (100.0% cancel ratio).',
    review_status: 'OPEN',
    created_at: '2026-11-11T06:26:41Z',
  },
  {
    alert_id: 32,
    trader_id: 3,
    trader_name: 'ShadowBot (High Volume)',
    order_id: 68,
    symbol: 'RELIANCE',
    rule_name: 'HIGH_VOLUME',
    severity: 'HIGH',
    risk_points: 65,
    description: 'Persistent abnormal volume detected: 10 large orders submitted in recent window (current qty: 150).',
    review_status: 'OPEN',
    created_at: '2026-11-11T06:26:41Z',
  },
  {
    alert_id: 31,
    trader_id: 4,
    trader_name: 'FlashCancel (Rapid Cancel)',
    order_id: 66,
    symbol: 'INFY',
    rule_name: 'HIGH_FREQUENCY',
    severity: 'MEDIUM',
    risk_points: 50,
    description: 'Elevated order frequency detected: 5 orders submitted within 10 seconds.',
    review_status: 'OPEN',
    created_at: '2026-11-11T06:26:41Z',
  },
  {
    alert_id: 1,
    trader_id: 4,
    trader_name: 'FlashCancel (Rapid Cancel)',
    order_id: 5,
    symbol: 'TCS',
    rule_name: 'REPEATED_CANCELLATION',
    severity: 'MEDIUM',
    risk_points: 45,
    description: 'Frequent order submission and cancellation: 3 cancellations in 60s (100.0% cancel ratio).',
    review_status: 'REVIEWED',
    created_at: '2026-11-04T09:30:05Z',
  },
];

export const MOCK_SIMULATION_STATUS = {
  is_running: false,
  tick_interval_ms: 2000,
  total_ticks: 10,
  total_orders_generated: 34,
  total_executions_generated: 24,
  total_cancellations_generated: 6,
  total_rejections: 0,
  total_system_faults: 0,
  total_errors: 0,
  last_tick_time: '2026-11-11T06:26:41Z',
  active_bots: [
    { trader_id: 2, name: 'AlphaBot (Normal Trader)', strategy: 'NORMAL', status: 'READY', orders: 10, cancels: 2 },
    { trader_id: 3, name: 'ShadowBot (High Volume)', strategy: 'HIGH_VOLUME', status: 'ARMED', orders: 12, cancels: 0 },
    { trader_id: 4, name: 'FlashCancel (Rapid Cancel)', strategy: 'RAPID_CANCEL', status: 'ARMED', orders: 8, cancels: 4 },
    { trader_id: 5, name: 'StochasticBot (Random)', strategy: 'RANDOM', status: 'READY', orders: 4, cancels: 0 },
  ],
};
