/**
 * TradeShield Central Application Context Provider
 * Connects React UI state to the C++17 Crow Backend and SQLite database.
 *
 * Strict Integration Rules:
 * - SQLite & C++ Core are the single source of truth for all live trading data.
 * - Live mode NEVER silently replaces failed responses with mock data.
 * - Explicit connection states: 'CONNECTED' | 'STANDALONE' | 'DISCONNECTED' | 'LOADING'
 * - Automatic controlled polling during active simulation runs.
 * - Intraday stock history & portfolio trajectory sourced directly from backend.
 */

import { useState, useEffect, useCallback, useMemo, useRef } from 'react';
import { TradeShieldContext } from './TradeShieldContext';
import { api } from '../services/api';
import {
  MOCK_STOCKS,
  MOCK_PORTFOLIO,
  MOCK_HOLDINGS,
  MOCK_ORDERS,
  MOCK_TRADES,
  MOCK_ALERTS,
  MOCK_SIMULATION_STATUS,
  MOCK_TRADERS,
} from '../data/mockData';

const STOCK_ACCENTS = {
  RELIANCE: '#F59E0B',
  TCS: '#3B82F6',
  INFY: '#8B5CF6',
  HDFCBANK: '#10B981',
  TATAMOTORS: '#EC4899',
  ICICIBANK: '#06B6D4',
};

const DEFAULT_SIMULATION = {
  is_running: false,
  tick_interval_ms: 2000,
  total_ticks: 0,
  total_orders_generated: 0,
  total_executions_generated: 0,
  total_cancellations_generated: 0,
  total_rejections: 0,
  total_system_faults: 0,
  total_errors: 0,
  last_tick_time: '',
  active_bots: [],
};

const DEFAULT_PORTFOLIO = {
  trader_id: 1,
  trader_name: 'Lakshya Khanna',
  available_cash: 50000000,
  cash_balance_paise: 50000000,
  invested_value: 0,
  total_invested_paise: 0,
  current_portfolio_value: 0,
  holdings_value_paise: 0,
  total_account_value: 50000000,
  total_value_paise: 50000000,
  total_unrealized_pnl: 0,
  unrealized_pnl_paise: 0,
  total_pnl_percent: 0,
  unrealized_pnl_percentage: 0,
  performance_history: [],
};

const DEFAULT_ALERT_STATS = {
  total_alerts: 0,
  open_alerts: 0,
  reviewed_alerts: 0,
  cleared_alerts: 0,
  high_severity_alerts: 0,
  medium_severity_alerts: 0,
  low_severity_alerts: 0,
  active_incidents: 0,
  high_critical_alerts: 0,
};

export function TradeShieldProvider({ children }) {
  // Connection state: 'LOADING' | 'CONNECTED' | 'STANDALONE' | 'DISCONNECTED'
  const [connectionStatus, setConnectionStatus] = useState('LOADING');
  const [currentTrader, setCurrentTrader] = useState({
    trader_id: 1,
    name: 'Lakshya Khanna',
    trader_type: 'HUMAN',
    cash_balance_paise: 50000000,
  });
  const [traders, setTraders] = useState([]);
  const [stocks, setStocks] = useState([]);
  const [portfolio, setPortfolio] = useState(DEFAULT_PORTFOLIO);
  const [holdings, setHoldings] = useState([]);
  const [orders, setOrders] = useState([]);
  const [trades, setTrades] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [alertStats, setAlertStats] = useState(DEFAULT_ALERT_STATS);
  const [riskProfiles, setRiskProfiles] = useState([]);
  const [simulation, setSimulation] = useState(DEFAULT_SIMULATION);
  const [isLoading, setIsLoading] = useState(false);
  const [isRefreshing, setIsRefreshing] = useState(false);
  const [isSimActionLoading, setIsSimActionLoading] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');
  const [toast, setToast] = useState(null);
  const [lastUpdateTime, setLastUpdateTime] = useState(new Date().toISOString());

  // References for polling and mounting
  const isPollingRef = useRef(false);
  const simulationRef = useRef(simulation);
  useEffect(() => {
    simulationRef.current = simulation;
  }, [simulation]);

  const stocksRef = useRef(stocks);
  useEffect(() => {
    stocksRef.current = stocks;
  }, [stocks]);

  const tradersRef = useRef(traders);
  useEffect(() => {
    tradersRef.current = traders;
  }, [traders]);

  const ordersRef = useRef(orders);
  useEffect(() => {
    ordersRef.current = orders;
  }, [orders]);

  const currentTraderRef = useRef(currentTrader);
  useEffect(() => {
    currentTraderRef.current = currentTrader;
  }, [currentTrader]);

  const alertsRef = useRef(alerts);
  useEffect(() => {
    alertsRef.current = alerts;
  }, [alerts]);

  // Show a notification toast
  const showToast = useCallback((message, type = 'info') => {
    setToast({ message, type, id: Date.now() });
    setTimeout(() => {
      setToast((prev) => (prev?.id === Date.now() ? null : prev));
    }, 4500);
  }, []);

  // Normalizer: Normalize stocks from C++ backend
  const normalizeStocks = useCallback((backendStocks) => {
    if (!Array.isArray(backendStocks)) return [];

    // Derive active alert count per stock symbol for dynamic Shield risk tag
    const highRiskStockSymbols = new Set(
      (alertsRef.current || [])
        .filter((a) => (a.severity === 'HIGH' || a.severity === 'CRITICAL') && a.review_status === 'OPEN')
        .map((a) => a.symbol)
        .filter(Boolean)
    );

    return backendStocks.map((s) => {
      const pricePaise = s.current_price_paise !== undefined ? s.current_price_paise : (s.current_price || 0);
      const openPaise = s.open_price_paise !== undefined ? s.open_price_paise : pricePaise;

      let change24h = s.change_24h !== undefined ? Number(s.change_24h) : 0;
      if (s.change_24h === undefined && openPaise > 0) {
        change24h = Number((((pricePaise - openPaise) / openPaise) * 100).toFixed(2));
      }

      // Enforce 24h low <= last price <= 24h high mathematically
      const rawHigh = s.high_24h_paise !== undefined ? s.high_24h_paise : pricePaise;
      const rawLow = s.low_24h_paise !== undefined ? s.low_24h_paise : pricePaise;
      const safeHigh = Math.max(rawHigh, pricePaise);
      const safeLow = Math.min(rawLow, pricePaise);

      const volume = s.volume !== undefined ? s.volume : 0;
      const accent = STOCK_ACCENTS[s.symbol] || s.accent_color || '#8B5CF6';

      // History provided directly by backend or generated from bounds
      const history = Array.isArray(s.history) && s.history.length > 0
        ? s.history
        : [
            { time: '09:15', price: Number((openPaise / 100).toFixed(2)) },
            { time: '15:30', price: Number((pricePaise / 100).toFixed(2)) },
          ];

      return {
        stock_id: s.stock_id,
        symbol: s.symbol,
        name: s.company_name || s.name || s.symbol,
        company_name: s.company_name || s.name || s.symbol,
        current_price: pricePaise,
        current_price_paise: pricePaise,
        open_price: openPaise,
        open_price_paise: openPaise,
        high_24h: safeHigh,
        high_24h_paise: safeHigh,
        low_24h: safeLow,
        low_24h_paise: safeLow,
        change_24h: change24h,
        change_7d: s.change_7d !== undefined ? s.change_7d : Number((change24h * 1.5).toFixed(2)),
        volume: volume,
        accent_color: accent,
        shield_risk: highRiskStockSymbols.has(s.symbol) ? 'HIGH' : 'LOW',
        is_simulated: Boolean(s.is_simulated),
        history: history,
      };
    });
  }, []);

  // Normalizer: Normalize portfolio from C++ backend
  const normalizePortfolio = useCallback((p) => {
    if (!p) return DEFAULT_PORTFOLIO;
    const cash = p.cash_balance_paise !== undefined ? p.cash_balance_paise : (p.available_cash || 0);
    const invested = p.total_invested_paise !== undefined ? p.total_invested_paise : (p.invested_value || 0);
    const holdingsVal = p.holdings_value_paise !== undefined ? p.holdings_value_paise : (p.current_portfolio_value || 0);
    const totalVal = p.total_value_paise !== undefined ? p.total_value_paise : (cash + holdingsVal);
    const pnlPaise = p.unrealized_pnl_paise !== undefined ? p.unrealized_pnl_paise : (p.total_unrealized_pnl || 0);
    const pnlPct = p.unrealized_pnl_percentage !== undefined ? p.unrealized_pnl_percentage : (p.total_pnl_percent || 0);

    // Performance history: genuine snapshots persisted in SQLite
    const perfHistory = Array.isArray(p.performance_history) ? p.performance_history : [];

    return {
      trader_id: p.trader_id || 1,
      trader_name: p.trader_name || 'Lakshya Khanna',
      available_cash: cash,
      cash_balance_paise: cash,
      invested_value: invested,
      total_invested_paise: invested,
      current_portfolio_value: holdingsVal,
      holdings_value_paise: holdingsVal,
      total_account_value: totalVal,
      total_value_paise: totalVal,
      total_unrealized_pnl: pnlPaise,
      unrealized_pnl_paise: pnlPaise,
      total_pnl_percent: pnlPct,
      unrealized_pnl_percentage: pnlPct,
      performance_history: perfHistory,
    };
  }, []);

  // Normalizer: Normalize positions from C++ backend
  const normalizePositions = useCallback((rawPositions) => {
    if (!Array.isArray(rawPositions)) return [];
    return rawPositions.map((p) => {
      const avgPrice = p.average_buy_price_paise !== undefined ? p.average_buy_price_paise : (p.average_buy_price || 0);
      const currPrice = p.current_price_paise !== undefined ? p.current_price_paise : (p.current_price || 0);
      const invVal = p.total_invested_paise !== undefined ? p.total_invested_paise : (p.invested_value || 0);
      const currVal = p.current_value_paise !== undefined ? p.current_value_paise : (p.current_value || 0);
      const pnl = p.unrealized_pnl_paise !== undefined ? p.unrealized_pnl_paise : (p.unrealized_pnl || 0);
      const pnlPct = p.unrealized_pnl_percentage !== undefined ? p.unrealized_pnl_percentage : (p.pnl_percent || 0);

      return {
        position_id: p.position_id,
        stock_id: p.stock_id,
        symbol: p.symbol,
        name: p.company_name || p.name || p.symbol,
        quantity: p.quantity,
        average_buy_price: avgPrice,
        average_buy_price_paise: avgPrice,
        current_price: currPrice,
        current_price_paise: currPrice,
        invested_value: invVal,
        current_value: currVal,
        unrealized_pnl: pnl,
        pnl_percent: pnlPct,
      };
    });
  }, []);

  // Normalizer: Normalize orders from C++ backend
  const normalizeOrders = useCallback((rawOrders, stockList, traderList) => {
    if (!Array.isArray(rawOrders)) return [];
    const stockMap = new Map((stockList || []).map((s) => [s.stock_id, s.symbol]));
    const traderMap = new Map((traderList || []).map((t) => [t.trader_id, t.name]));

    return rawOrders.map((ord) => ({
      order_id: ord.order_id,
      trader_id: ord.trader_id,
      trader_name: traderMap.get(ord.trader_id) || (ord.trader_id === 1 ? 'Lakshya Khanna' : `Trader #${ord.trader_id}`),
      stock_id: ord.stock_id,
      symbol: ord.symbol || stockMap.get(ord.stock_id) || `STOCK #${ord.stock_id}`,
      side: ord.side,
      type: 'LIMIT',
      quantity: ord.quantity,
      price: ord.limit_price_paise !== undefined ? ord.limit_price_paise : ord.price,
      limit_price_paise: ord.limit_price_paise !== undefined ? ord.limit_price_paise : ord.price,
      status: ord.status,
      created_at: ord.created_at,
      updated_at: ord.updated_at,
    }));
  }, []);

  // Normalizer: Normalize trades from C++ backend
  const normalizeTrades = useCallback((rawTrades, orderList) => {
    if (!Array.isArray(rawTrades)) return [];
    const orderMap = new Map((orderList || []).map((o) => [o.order_id, o]));

    return rawTrades.map((trd) => {
      const parentOrder = orderMap.get(trd.order_id);
      const qty = trd.executed_quantity !== undefined ? trd.executed_quantity : (trd.quantity || 0);
      const execPrice = trd.executed_price_paise !== undefined ? trd.executed_price_paise : (trd.execution_price || 0);
      const totalVal = trd.total_value_paise !== undefined ? trd.total_value_paise : (trd.trade_value || 0);

      return {
        trade_id: trd.trade_id,
        order_id: trd.order_id,
        symbol: trd.symbol || parentOrder?.symbol || 'EQUITY',
        buyer_name: trd.buyer_name || (parentOrder?.side === 'BUY' ? parentOrder.trader_name : 'Exchange Match'),
        seller_name: trd.seller_name || (parentOrder?.side === 'SELL' ? parentOrder.trader_name : 'Exchange Counterparty'),
        quantity: qty,
        execution_price: execPrice,
        trade_value: totalVal,
        executed_at: trd.executed_at,
      };
    });
  }, []);

  // Normalizer: Normalize alerts from C++ backend
  const normalizeAlerts = useCallback((rawAlerts, stockList, traderList) => {
    if (!rawAlerts) return [];
    const list = Array.isArray(rawAlerts) ? rawAlerts : (rawAlerts?.alerts || []);
    const stockMap = new Map((stockList || []).map((s) => [s.stock_id, s.symbol]));
    const traderMap = new Map((traderList || []).map((t) => [t.trader_id, t.name]));

    return list.map((a) => ({
      ...a,
      trader_name: a.trader_name || traderMap.get(a.trader_id) || `Trader #${a.trader_id}`,
      symbol: a.symbol || (a.stock_id ? stockMap.get(a.stock_id) : 'NSE:EQ'),
      occurrence_count: a.occurrence_count || 1,
    }));
  }, []);

  // Normalizer: Normalize aggregate Shield alert statistics
  const normalizeAlertStats = useCallback((rawStats, alertList = []) => {
    const list = Array.isArray(alertList) ? alertList : [];
    if (rawStats && typeof rawStats === 'object') {
      return {
        total_alerts: Number(rawStats.total_alerts) || list.length,
        open_alerts: Number(rawStats.open_alerts) || list.filter((a) => a.review_status === 'OPEN').length,
        reviewed_alerts: Number(rawStats.reviewed_alerts) || list.filter((a) => a.review_status === 'REVIEWED').length,
        cleared_alerts: Number(rawStats.cleared_alerts) || list.filter((a) => a.review_status === 'CLEARED').length,
        high_severity_alerts: Number(rawStats.high_severity_alerts) || 0,
        medium_severity_alerts: Number(rawStats.medium_severity_alerts) || 0,
        low_severity_alerts: Number(rawStats.low_severity_alerts) || 0,
        active_incidents: Number(rawStats.active_incidents) || 0,
        high_critical_alerts: Number(rawStats.high_critical_alerts ?? rawStats.high_severity_alerts ?? 0),
      };
    }
    return {
      total_alerts: list.length,
      open_alerts: list.filter((a) => a.review_status === 'OPEN').length,
      reviewed_alerts: list.filter((a) => a.review_status === 'REVIEWED').length,
      cleared_alerts: list.filter((a) => a.review_status === 'CLEARED').length,
      high_severity_alerts: list.filter((a) => a.severity === 'HIGH').length,
      medium_severity_alerts: list.filter((a) => a.severity === 'MEDIUM').length,
      low_severity_alerts: list.filter((a) => a.severity === 'LOW').length,
      active_incidents: 0,
      high_critical_alerts: 0,
    };
  }, []);

  // Normalizer: Normalize simulation status metrics
  const normalizeSimulationStatus = useCallback((rawStatus) => {
    if (!rawStatus || typeof rawStatus !== 'object') return DEFAULT_SIMULATION;
    return {
      is_running: Boolean(rawStatus.is_running),
      tick_interval_ms: Number(rawStatus.tick_interval_ms) || 2000,
      total_ticks: Number(rawStatus.total_ticks) || 0,
      total_orders_generated: Number(rawStatus.total_orders_generated) || 0,
      total_executions_generated: Number(rawStatus.total_executions_generated) || 0,
      total_cancellations_generated: Number(rawStatus.total_cancellations_generated) || 0,
      total_rejections: Number(rawStatus.total_rejections ?? rawStatus.total_errors ?? 0),
      total_system_faults: Number(rawStatus.total_system_faults ?? 0),
      total_errors: Number(rawStatus.total_errors ?? 0),
      last_tick_time: rawStatus.last_tick_time || '',
      active_bots: Array.isArray(rawStatus.active_bots) ? rawStatus.active_bots : [],
    };
  }, []);

  // Normalizer: Normalize trader risk rankings
  const normalizeRiskProfiles = useCallback((rawProfiles) => {
    if (!rawProfiles) return [];
    if (Array.isArray(rawProfiles)) return rawProfiles;
    if (Array.isArray(rawProfiles.risk_profiles)) return rawProfiles.risk_profiles;
    return [];
  }, []);

  // Primary Refresh Function (fetches all state from C++ backend)
  const refreshData = useCallback(async () => {
    try {
      setIsRefreshing(true);
      setIsLoading(true);

      // 1. Probe Health
      const health = await api.getHealth();
      if (!health.isConnected) {
        setConnectionStatus('DISCONNECTED');
        setIsLoading(false);
        setIsRefreshing(false);
        return;
      }
      setConnectionStatus('CONNECTED');

      const traderId = currentTraderRef.current?.trader_id || 1;

      // 2. Fetch parallel entities from real backend
      const [stocksRes, tradersRes, portRes, posRes, ordRes, trdRes, altRes, altStatsRes, riskRes, simRes] =
        await Promise.allSettled([
          api.getStocks(false),
          api.getTraders(false),
          api.getPortfolio(traderId, false),
          api.getPositions(traderId, false),
          api.getOrders({}, false),
          api.getTrades({}, false),
          api.getAlerts({}, false),
          api.getAlertStats(false),
          api.getRiskScores(false),
          api.getSimulationStatus(false),
        ]);

      let loadedStocks = stocksRef.current;
      if (stocksRes.status === 'fulfilled' && stocksRes.value?.data) {
        loadedStocks = normalizeStocks(stocksRes.value.data);
        setStocks(loadedStocks);
      }

      let loadedTraders = tradersRef.current;
      if (tradersRes.status === 'fulfilled' && tradersRes.value?.data) {
        loadedTraders = tradersRes.value.data;
        setTraders(loadedTraders);
        const human = loadedTraders.find((t) => t.trader_type === 'HUMAN' || t.trader_id === 1);
        if (human) setCurrentTrader(human);
      }

      if (posRes.status === 'fulfilled' && posRes.value?.data) {
        setHoldings(normalizePositions(posRes.value.data));
      }

      if (portRes.status === 'fulfilled' && portRes.value?.data) {
        setPortfolio(normalizePortfolio(portRes.value.data));
      }

      let loadedOrders = ordersRef.current;
      if (ordRes.status === 'fulfilled' && ordRes.value?.data) {
        loadedOrders = normalizeOrders(ordRes.value.data, loadedStocks, loadedTraders);
        setOrders(loadedOrders);
      }

      if (trdRes.status === 'fulfilled' && trdRes.value?.data) {
        setTrades(normalizeTrades(trdRes.value.data, loadedOrders));
      }

      let currentAlertsList = [];
      if (altRes.status === 'fulfilled' && altRes.value?.data) {
        currentAlertsList = normalizeAlerts(altRes.value.data, loadedStocks, loadedTraders);
        setAlerts(currentAlertsList);
      }

      if (altStatsRes.status === 'fulfilled' && altStatsRes.value?.data) {
        setAlertStats(normalizeAlertStats(altStatsRes.value.data, currentAlertsList));
      }

      if (riskRes.status === 'fulfilled' && riskRes.value?.data) {
        setRiskProfiles(normalizeRiskProfiles(riskRes.value.data));
      }

      if (simRes.status === 'fulfilled' && simRes.value?.data) {
        setSimulation(normalizeSimulationStatus(simRes.value.data));
      }

      setLastUpdateTime(new Date().toISOString());
    } catch (err) {
      console.error('Refresh error:', err);
      setConnectionStatus('DISCONNECTED');
    } finally {
      setIsRefreshing(false);
      setIsLoading(false);
    }
  }, [
    normalizeAlerts,
    normalizeAlertStats,
    normalizeOrders,
    normalizePortfolio,
    normalizePositions,
    normalizeRiskProfiles,
    normalizeSimulationStatus,
    normalizeStocks,
    normalizeTrades,
  ]);

  // Initial load
  useEffect(() => {
    let active = true;
    const init = async () => {
      await Promise.resolve();
      if (active) {
        await refreshData();
      }
    };
    init();
    return () => {
      active = false;
    };
  }, [refreshData]);

  // Controlled Automated Polling (Refreshes telemetry during simulation runs)
  useEffect(() => {
    let isMounted = true;

    const pollInterval = setInterval(async () => {
      if (!isMounted || document.hidden || isPollingRef.current || connectionStatus !== 'CONNECTED') {
        return;
      }

      isPollingRef.current = true;
      try {
        const isSimRunning = Boolean(simulationRef.current?.is_running);
        const traderId = currentTraderRef.current?.trader_id || 1;

        if (isSimRunning) {
          const [simRes, altRes, altStatsRes, riskRes, ordRes, trdRes, portRes, stocksRes] = await Promise.allSettled([
            api.getSimulationStatus(false),
            api.getAlerts({}, false),
            api.getAlertStats(false),
            api.getRiskScores(false),
            api.getOrders({ limit: 100 }, false),
            api.getTrades({ limit: 100 }, false),
            api.getPortfolio(traderId, false),
            api.getStocks(false),
          ]);

          if (!isMounted) return;

          if (simRes.status === 'fulfilled' && simRes.value?.data) {
            setSimulation(normalizeSimulationStatus(simRes.value.data));
          }

          let currentAlertsList = [];
          if (altRes.status === 'fulfilled' && altRes.value?.data) {
            currentAlertsList = normalizeAlerts(altRes.value.data, stocksRef.current, tradersRef.current);
            setAlerts(currentAlertsList);
          }

          if (altStatsRes.status === 'fulfilled' && altStatsRes.value?.data) {
            setAlertStats(normalizeAlertStats(altStatsRes.value.data, currentAlertsList));
          }

          if (riskRes.status === 'fulfilled' && riskRes.value?.data) {
            setRiskProfiles(normalizeRiskProfiles(riskRes.value.data));
          }

          if (stocksRes.status === 'fulfilled' && stocksRes.value?.data) {
            setStocks(normalizeStocks(stocksRes.value.data));
          }

          let polledOrders = [];
          if (ordRes.status === 'fulfilled' && ordRes.value?.data) {
            polledOrders = normalizeOrders(ordRes.value.data, stocksRef.current, tradersRef.current);
            setOrders(polledOrders);
          }

          if (trdRes.status === 'fulfilled' && trdRes.value?.data) {
            setTrades(normalizeTrades(trdRes.value.data, polledOrders));
          }

          if (portRes.status === 'fulfilled' && portRes.value?.data) {
            setPortfolio(normalizePortfolio(portRes.value.data));
          }

          setLastUpdateTime(new Date().toISOString());
        } else {
          // Idle heartbeat (lightweight simulation status check)
          const simRes = await api.getSimulationStatus(false);
          if (isMounted && simRes?.data) {
            setSimulation(normalizeSimulationStatus(simRes.data));
            setLastUpdateTime(new Date().toISOString());
          }
        }
      } catch (err) {
        console.warn('Simulation telemetry polling tick warning:', err?.message || err);
      } finally {
        isPollingRef.current = false;
      }
    }, 1800);

    return () => {
      isMounted = false;
      clearInterval(pollInterval);
      isPollingRef.current = false;
    };
  }, [
    connectionStatus,
    normalizeAlerts,
    normalizeAlertStats,
    normalizeOrders,
    normalizePortfolio,
    normalizeRiskProfiles,
    normalizeSimulationStatus,
    normalizeStocks,
    normalizeTrades,
  ]);

  // 1. Submit Order
  const placeOrder = useCallback(
    async (orderData) => {
      const isStandalone = connectionStatus === 'STANDALONE';
      setIsLoading(true);

      try {
        const payload = {
          trader_id: currentTrader.trader_id,
          stock_id: orderData.stock_id,
          side: orderData.side,
          quantity: Number(orderData.quantity),
          limit_price_paise: Number(orderData.limit_price_paise || orderData.price_paise),
        };

        const res = await api.placeOrder(payload, isStandalone);
        await refreshData();

        const orderInfo = res.data?.order;
        showToast(
          `Order #${orderInfo?.order_id || 'Submitted'}: ${payload.side} ${payload.quantity} ${orderData.symbol} — ${orderInfo?.status || 'SUBMITTED'}`,
          'success'
        );
        return { success: true, data: res.data };
      } catch (err) {
        const message = err.message || 'Order rejected by TradingEngine';
        showToast(message, 'error');
        return { success: false, error: message };
      } finally {
        setIsLoading(false);
      }
    },
    [connectionStatus, currentTrader.trader_id, refreshData, showToast]
  );

  // 2. Cancel Order
  const cancelOrder = useCallback(
    async (orderId) => {
      const isStandalone = connectionStatus === 'STANDALONE';
      try {
        const res = await api.cancelOrder(orderId, isStandalone);
        await refreshData();
        showToast(res.data?.message || `Order #${orderId} cancelled`, 'info');
        return { success: true };
      } catch (err) {
        const message = err.message || `Failed to cancel Order #${orderId}`;
        showToast(message, 'error');
        return { success: false, error: message };
      }
    },
    [connectionStatus, refreshData, showToast]
  );

  // 3. Review Alert
  const reviewAlert = useCallback(
    async (alertId, status = 'REVIEWED') => {
      const isStandalone = connectionStatus === 'STANDALONE';
      const safeStatus = status === 'CLEARED' ? 'CLEARED' : 'REVIEWED';

      try {
        const res = await api.reviewAlert(alertId, safeStatus, isStandalone);
        await refreshData();
        showToast(res.data?.message || `Alert #${alertId} marked as ${safeStatus}`, 'success');
        return { success: true };
      } catch (err) {
        const message = err.message || `Failed to review Alert #${alertId}`;
        showToast(message, 'error');
        return { success: false, error: message };
      }
    },
    [connectionStatus, refreshData, showToast]
  );

  // 4. Start Simulation
  const startSimulation = useCallback(
    async (intervalMs = 2000) => {
      if (isSimActionLoading) return { success: false, error: 'Simulation operation already in progress' };
      setIsSimActionLoading(true);
      const isStandalone = connectionStatus === 'STANDALONE';
      try {
        const res = await api.startSimulation(intervalMs, isStandalone);
        setSimulation((prev) => ({
          ...prev,
          is_running: true,
          tick_interval_ms: intervalMs,
        }));
        showToast('Bot Simulation Engine started on C++ server', 'success');
        return { success: true, data: res.data };
      } catch (err) {
        const message = err.message || 'Failed to start Simulation Engine';
        showToast(message, 'error');
        return { success: false, error: message };
      } finally {
        setIsSimActionLoading(false);
      }
    },
    [connectionStatus, isSimActionLoading, showToast]
  );

  // 5. Stop Simulation
  const stopSimulation = useCallback(async () => {
    if (isSimActionLoading) return { success: false, error: 'Simulation operation already in progress' };
    setIsSimActionLoading(true);
    const isStandalone = connectionStatus === 'STANDALONE';
    try {
      const res = await api.stopSimulation(isStandalone);
      setSimulation((prev) => ({
        ...prev,
        is_running: false,
      }));
      showToast('Bot Simulation Engine stopped', 'info');
      await refreshData();
      return { success: true, data: res.data };
    } catch (err) {
      const message = err.message || 'Failed to stop Simulation Engine';
      showToast(message, 'error');
      return { success: false, error: message };
    } finally {
      setIsSimActionLoading(false);
    }
  }, [connectionStatus, isSimActionLoading, refreshData, showToast]);

  // 6. Reset Simulation Metrics
  const resetSimulation = useCallback(async () => {
    if (isSimActionLoading) return { success: false, error: 'Simulation operation already in progress' };
    setIsSimActionLoading(true);
    const isStandalone = connectionStatus === 'STANDALONE';
    try {
      const res = await api.resetSimulation(isStandalone);
      setSimulation((prev) => ({
        ...prev,
        total_ticks: 0,
        total_orders_generated: 0,
        total_executions_generated: 0,
        total_cancellations_generated: 0,
        total_rejections: 0,
        total_system_faults: 0,
        total_errors: 0,
      }));
      showToast(res.data?.message || 'Simulation counters reset', 'info');
      return { success: true };
    } catch (err) {
      const message = err.message || 'Failed to reset simulation metrics';
      showToast(message, 'error');
      return { success: false, error: message };
    } finally {
      setIsSimActionLoading(false);
    }
  }, [connectionStatus, isSimActionLoading, showToast]);

  // Explicit Toggle Mode between LIVE and STANDALONE
  const toggleStandaloneMode = useCallback(() => {
    if (connectionStatus === 'STANDALONE') {
      refreshData();
    } else {
      setConnectionStatus('STANDALONE');
      setStocks(MOCK_STOCKS);
      setPortfolio(MOCK_PORTFOLIO);
      setHoldings(MOCK_HOLDINGS);
      setOrders(MOCK_ORDERS);
      setTrades(MOCK_TRADES);
      setAlerts(MOCK_ALERTS);
      setTraders(MOCK_TRADERS);
      setSimulation(MOCK_SIMULATION_STATUS);
      showToast('Switched to Standalone Mock Mode', 'info');
    }
  }, [connectionStatus, refreshData, showToast]);

  const value = useMemo(
    () => ({
      connectionStatus,
      setConnectionStatus,
      isBackendConnected: connectionStatus === 'CONNECTED',
      toggleStandaloneMode,
      currentTrader,
      setCurrentTrader,
      traders,
      stocks,
      portfolio,
      holdings,
      orders,
      trades,
      alerts,
      alertStats,
      riskProfiles,
      simulation,
      isLoading,
      isRefreshing,
      isSimActionLoading,
      searchQuery,
      setSearchQuery,
      toast,
      showToast,
      lastUpdateTime,
      refreshData,
      placeOrder,
      cancelOrder,
      reviewAlert,
      startSimulation,
      stopSimulation,
      resetSimulation,
    }),
    [
      connectionStatus,
      toggleStandaloneMode,
      currentTrader,
      traders,
      stocks,
      portfolio,
      holdings,
      orders,
      trades,
      alerts,
      alertStats,
      riskProfiles,
      simulation,
      isLoading,
      isRefreshing,
      isSimActionLoading,
      searchQuery,
      toast,
      showToast,
      lastUpdateTime,
      refreshData,
      placeOrder,
      cancelOrder,
      reviewAlert,
      startSimulation,
      stopSimulation,
      resetSimulation,
    ]
  );

  return <TradeShieldContext.Provider value={value}>{children}</TradeShieldContext.Provider>;
}
