/**
 * TradeShield Central API Service
 * Connects directly to the C++17 Crow Backend (http://localhost:8080/api).
 *
 * Strict Error & Data Integrity:
 * - Live data originates from C++ backend and SQLite database.
 * - Live mode NEVER silently replaces failed responses with mock data.
 * - mockData.js is isolated strictly for explicitly selected standalone/demo mode.
 */

import axios from 'axios';
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

const BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:8080/api';

const apiClient = axios.create({
  baseURL: BASE_URL,
  timeout: 5000,
  headers: {
    'Content-Type': 'application/json',
  },
});

export const api = {
  getBaseUrl() {
    return BASE_URL;
  },

  // 1. Health Probe
  async getHealth() {
    try {
      const res = await apiClient.get('/health');
      return { data: res.data, isMock: false, isConnected: true };
    } catch (err) {
      return {
        data: null,
        isMock: false,
        isConnected: false,
        error: err.response?.data?.error || err.message,
      };
    }
  },

  // 2. Stocks
  async getStocks(isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_STOCKS, isMock: true };
    }
    try {
      const res = await apiClient.get('/stocks');
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve stocks', { cause: err });
    }
  },

  async getStockById(id, isStandalone = false) {
    if (isStandalone) {
      const stock = MOCK_STOCKS.find((s) => s.stock_id === Number(id)) || MOCK_STOCKS[0];
      return { data: stock, isMock: true };
    }
    const res = await apiClient.get('/stocks');
    if (Array.isArray(res.data)) {
      const found = res.data.find((s) => s.stock_id === Number(id));
      if (found) return { data: found, isMock: false };
    }
    return { data: res.data, isMock: false };
  },

  async getStockHistory(stockId, isStandalone = false) {
    if (isStandalone) {
      const stock = MOCK_STOCKS.find((s) => s.stock_id === Number(stockId)) || MOCK_STOCKS[0];
      return { data: { history: stock.history, is_simulated: true }, isMock: true };
    }
    try {
      const res = await apiClient.get(`/stocks/${stockId}/history`);
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || `Failed to retrieve history for stock #${stockId}`, { cause: err });
    }
  },

  // 3. Traders
  async getTraders(isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_TRADERS, isMock: true };
    }
    try {
      const res = await apiClient.get('/traders');
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve traders', { cause: err });
    }
  },

  // 4. Portfolio & Positions
  async getPortfolio(traderId = 1, isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_PORTFOLIO, isMock: true };
    }
    try {
      const res = await apiClient.get(`/portfolio?trader_id=${traderId}`);
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve portfolio', { cause: err });
    }
  },

  async getPositions(traderId = 1, isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_HOLDINGS, isMock: true };
    }
    try {
      const res = await apiClient.get(`/positions?trader_id=${traderId}`);
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve positions', { cause: err });
    }
  },

  // 5. Orders (Read & Write)
  async getOrders(params = {}, isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_ORDERS, isMock: true };
    }
    try {
      const res = await apiClient.get('/orders', { params });
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve orders', { cause: err });
    }
  },

  async getOrderById(orderId) {
    const res = await apiClient.get(`/orders/${orderId}`);
    return { data: res.data, isMock: false };
  },

  /**
   * Submit Order directly to C++ Trading Engine: POST /api/orders
   * NEVER falls back silently on error.
   */
  async placeOrder(orderPayload, isStandaloneMode = false) {
    const payload = {
      trader_id: Number(orderPayload.trader_id) || 1,
      stock_id: Number(orderPayload.stock_id),
      side: orderPayload.side,
      quantity: Number(orderPayload.quantity),
      limit_price_paise: Number(orderPayload.limit_price_paise || orderPayload.price_paise),
    };

    if (isStandaloneMode) {
      const mockOrder = {
        order_id: Math.floor(Date.now() / 1000) % 100000,
        trader_id: payload.trader_id,
        trader_name: 'Lakshya Khanna',
        stock_id: payload.stock_id,
        symbol: orderPayload.symbol || 'STOCK',
        side: payload.side,
        quantity: payload.quantity,
        limit_price_paise: payload.limit_price_paise,
        status: 'EXECUTED',
        created_at: new Date().toISOString(),
        updated_at: new Date().toISOString(),
      };
      return {
        data: {
          success: true,
          message: 'Order simulated in standalone mock mode',
          order: mockOrder,
        },
        isMock: true,
      };
    }

    try {
      const res = await apiClient.post('/orders', payload);
      return { data: res.data, isMock: false };
    } catch (err) {
      const errorMsg =
        err.response?.data?.error ||
        err.response?.data?.message ||
        err.message ||
        'Order placement rejected by Trading Engine';
      throw new Error(errorMsg, { cause: err });
    }
  },

  /**
   * Cancel Order: POST /api/orders/<int>/cancel
   * NEVER falls back silently on error.
   */
  async cancelOrder(orderId, isStandaloneMode = false) {
    if (isStandaloneMode) {
      return {
        data: { success: true, message: `Order #${orderId} cancelled in mock mode` },
        isMock: true,
      };
    }

    try {
      const res = await apiClient.post(`/orders/${orderId}/cancel`);
      return { data: res.data, isMock: false };
    } catch (err) {
      const errorMsg =
        err.response?.data?.error ||
        err.response?.data?.message ||
        err.message ||
        `Cancellation of Order #${orderId} failed`;
      throw new Error(errorMsg, { cause: err });
    }
  },

  // 6. Executed Trades
  async getTrades(params = {}, isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_TRADES, isMock: true };
    }
    try {
      const res = await apiClient.get('/trades', { params });
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve trades', { cause: err });
    }
  },

  // 7. Shield Alerts & Risk Intelligence
  async getAlertStats(isStandalone = false) {
    if (isStandalone) {
      return {
        data: {
          total_alerts: MOCK_ALERTS.length,
          open_alerts: MOCK_ALERTS.filter((a) => a.review_status === 'OPEN').length,
          reviewed_alerts: MOCK_ALERTS.filter((a) => a.review_status === 'REVIEWED').length,
          cleared_alerts: 0,
          high_severity_alerts: MOCK_ALERTS.filter((a) => a.severity === 'HIGH').length,
          medium_severity_alerts: MOCK_ALERTS.filter((a) => a.severity === 'MEDIUM').length,
          low_severity_alerts: MOCK_ALERTS.filter((a) => a.severity === 'LOW').length,
          active_incidents: MOCK_ALERTS.filter((a) => a.review_status === 'OPEN').length,
        },
        isMock: true,
      };
    }
    try {
      const res = await apiClient.get('/alerts/stats');
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve alert statistics', { cause: err });
    }
  },

  async getAlerts(params = {}, isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_ALERTS, pagination: { total_count: MOCK_ALERTS.length }, isMock: true };
    }
    try {
      const res = await apiClient.get('/alerts', { params });
      const raw = res.data;
      const list = Array.isArray(raw) ? raw : (raw?.alerts || []);
      return { data: list, pagination: raw, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve alerts', { cause: err });
    }
  },

  async getAlertById(alertId) {
    const res = await apiClient.get(`/alerts/${alertId}`);
    return { data: res.data, isMock: false };
  },

  /**
   * Review Alert: POST /api/alerts/<int>/review with {"status": "REVIEWED" | "CLEARED"}
   * NEVER falls back silently on error.
   */
  async reviewAlert(alertId, status = 'REVIEWED', isStandaloneMode = false) {
    const safeStatus = (status === 'CLEARED' || status === 'REVIEWED') ? status : 'REVIEWED';

    if (isStandaloneMode) {
      return {
        data: { success: true, message: `Alert #${alertId} marked as ${safeStatus} in mock mode` },
        isMock: true,
      };
    }

    try {
      const res = await apiClient.post(`/alerts/${alertId}/review`, { status: safeStatus });
      return { data: res.data, isMock: false };
    } catch (err) {
      const errorMsg =
        err.response?.data?.error ||
        err.response?.data?.message ||
        err.message ||
        `Failed to review Alert #${alertId}`;
      throw new Error(errorMsg, { cause: err });
    }
  },

  async getRiskScores(isStandalone = false) {
    if (isStandalone) {
      return {
        data: [
          { trader_id: 4, trader_name: 'FlashCancel (Rapid Cancel)', overall_risk_score: 70, risk_tier: 'HIGH', total_alerts: 2, unreviewed_alerts: 1, triggered_rules: ['REPEATED_CANCELLATION'] },
          { trader_id: 3, trader_name: 'ShadowBot (High Volume)', overall_risk_score: 65, risk_tier: 'HIGH', total_alerts: 1, unreviewed_alerts: 1, triggered_rules: ['HIGH_VOLUME'] },
          { trader_id: 5, trader_name: 'StochasticBot (Random)', overall_risk_score: 15, risk_tier: 'LOW', total_alerts: 0, unreviewed_alerts: 0, triggered_rules: [] },
          { trader_id: 2, trader_name: 'AlphaBot (Normal Trader)', overall_risk_score: 0, risk_tier: 'LOW', total_alerts: 0, unreviewed_alerts: 0, triggered_rules: [] },
          { trader_id: 1, trader_name: 'Lakshya Khanna', overall_risk_score: 0, risk_tier: 'LOW', total_alerts: 0, unreviewed_alerts: 0, triggered_rules: [] },
        ],
        isMock: true,
      };
    }
    try {
      const res = await apiClient.get('/risk');
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve risk profiles', { cause: err });
    }
  },

  async getTraderRiskById(traderId) {
    const res = await apiClient.get(`/risk/${traderId}`);
    return { data: res.data, isMock: false };
  },

  // 8. Bot Simulation Controls (Strict write execution)
  async getSimulationStatus(isStandalone = false) {
    if (isStandalone) {
      return { data: MOCK_SIMULATION_STATUS, isMock: true };
    }
    try {
      const res = await apiClient.get('/simulation/status');
      return { data: res.data, isMock: false };
    } catch (err) {
      throw new Error(err.response?.data?.error || err.message || 'Failed to retrieve simulation status', { cause: err });
    }
  },

  async startSimulation(tickIntervalMs = 2000, isStandaloneMode = false) {
    if (isStandaloneMode) {
      return {
        data: { success: true, message: 'Simulation started in mock mode', is_running: true, tick_interval_ms: tickIntervalMs },
        isMock: true,
      };
    }

    try {
      const res = await apiClient.post('/simulation/start', { tick_interval_ms: Number(tickIntervalMs) });
      return { data: res.data, isMock: false };
    } catch (err) {
      const errorMsg =
        err.response?.data?.error ||
        err.response?.data?.message ||
        err.message ||
        'Failed to start Simulation Engine';
      throw new Error(errorMsg, { cause: err });
    }
  },

  async stopSimulation(isStandaloneMode = false) {
    if (isStandaloneMode) {
      return {
        data: { success: true, message: 'Simulation stopped in mock mode', is_running: false },
        isMock: true,
      };
    }

    try {
      const res = await apiClient.post('/simulation/stop');
      return { data: res.data, isMock: false };
    } catch (err) {
      const errorMsg =
        err.response?.data?.error ||
        err.response?.data?.message ||
        err.message ||
        'Failed to stop Simulation Engine';
      throw new Error(errorMsg, { cause: err });
    }
  },

  async resetSimulation(isStandaloneMode = false) {
    if (isStandaloneMode) {
      return {
        data: { success: true, message: 'Simulation metrics reset in mock mode' },
        isMock: true,
      };
    }

    try {
      const res = await apiClient.post('/simulation/reset');
      return { data: res.data, isMock: false };
    } catch (err) {
      const errorMsg =
        err.response?.data?.error ||
        err.response?.data?.message ||
        err.message ||
        'Failed to reset simulation metrics';
      throw new Error(errorMsg, { cause: err });
    }
  },
};

export default api;
