/**
 * TradeShield Trading Execution Terminal (Stage 8)
 * Connects directly to the C++ TradingEngine (POST /api/orders & POST /api/orders/:id/cancel).
 *
 * Supports limit orders:
 * - Marketable Limit Orders: Limit price set at or better than market price (executes immediately).
 * - Resting Limit Orders: Limit price set beyond market price (remains pending in order book).
 */

import { useState } from 'react';
import { useLocation } from 'react-router-dom';
import {
  AlertCircle,
  X,
  Zap,
} from 'lucide-react';
import { useTradeShield } from '../context/useTradeShield';
import { formatPaiseToINR } from '../utils/formatters';

export default function Trading() {
  const location = useLocation();
  const {
    stocks,
    portfolio,
    placeOrder,
    orders,
    cancelOrder,
    isLoading,
    isBackendConnected,
  } = useTradeShield();

  // Initial stock selection
  const initialSymbol = location.state?.preselectStock || stocks[0]?.symbol || 'RELIANCE';
  const initialStock = stocks.find((s) => s.symbol === initialSymbol) || stocks[0];

  // Active form state
  const [selectedSymbol, setSelectedSymbol] = useState(initialSymbol);
  const [side, setSide] = useState('BUY');
  const [quantity, setQuantity] = useState(10);
  const [priceRupees, setPriceRupees] = useState(
    initialStock ? (initialStock.current_price_paise || initialStock.current_price) / 100 : 2450.0
  );
  const [showConfirmModal, setShowConfirmModal] = useState(false);
  const [orderError, setOrderError] = useState(null);

  const activeStock = stocks.find((s) => s.symbol === selectedSymbol) || stocks[0];
  const marketPricePaise = activeStock?.current_price_paise || activeStock?.current_price || 0;

  const handleSymbolChange = (newSymbol) => {
    setSelectedSymbol(newSymbol);
    const stock = stocks.find((s) => s.symbol === newSymbol);
    if (stock) {
      const price = (stock.current_price_paise || stock.current_price) / 100;
      setPriceRupees(price);
    }
  };

  // Quick set to current market price
  const handleSetMarketPrice = () => {
    if (activeStock) {
      setPriceRupees(marketPricePaise / 100);
    }
  };

  // Derived financials
  const pricePaise = Math.round(priceRupees * 100);
  const estimatedTotalPaise = quantity * pricePaise;
  const availableCashPaise = portfolio?.available_cash || portfolio?.cash_balance_paise || 0;
  const isInsufficientCash = side === 'BUY' && estimatedTotalPaise > availableCashPaise;
  const isValidQuantity = Number.isInteger(Number(quantity)) && quantity > 0;
  const isValidPrice = priceRupees > 0;
  const canSubmit = Boolean(activeStock) && isValidQuantity && isValidPrice && !isInsufficientCash;

  // Determine if limit price makes the order immediately marketable
  const isMarketable =
    side === 'BUY'
      ? pricePaise >= marketPricePaise
      : pricePaise <= marketPricePaise;

  const handleOrderSubmit = async () => {
    if (!canSubmit) return;
    setOrderError(null);
    setShowConfirmModal(false);

    const res = await placeOrder({
      stock_id: activeStock.stock_id,
      symbol: activeStock.symbol,
      side,
      quantity,
      limit_price_paise: pricePaise,
    });

    if (!res.success) {
      setOrderError(res.error);
    }
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Title */}
      <div>
        <h1 className="page-title">
          Trade Execution Terminal
        </h1>
        <p className="page-subtitle">
          Submit limit orders directly through the C++ TradingEngine with atomic SQLite settlement.
        </p>
      </div>

      {/* Main Grid: Order Entry Form + Order Book / Recent Orders */}
      <div className="responsive-grid-trading">
        {/* Left: Professional Order Entry Form */}
        <div className="glass-card" style={{ padding: '22px 20px', minWidth: 0 }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '20px', flexWrap: 'wrap', gap: '8px' }}>
            <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff' }}>Order Entry</h3>
            <span className={`pill-badge ${isBackendConnected ? 'pill-green' : 'pill-purple'}`}>
              {isBackendConnected ? 'C++ ENGINE LIVE' : 'STANDALONE MODE'}
            </span>
          </div>

          {/* BUY / SELL Switcher */}
          <div
            style={{
              display: 'grid',
              gridTemplateColumns: '1fr 1fr',
              gap: '8px',
              backgroundColor: 'rgba(10, 9, 16, 0.6)',
              padding: '4px',
              borderRadius: '12px',
              marginBottom: '20px',
            }}
          >
            <button
              onClick={() => setSide('BUY')}
              style={{
                padding: '10px',
                borderRadius: '8px',
                border: 'none',
                fontWeight: 700,
                fontSize: '13px',
                cursor: 'pointer',
                backgroundColor: side === 'BUY' ? '#10b981' : 'transparent',
                color: side === 'BUY' ? '#ffffff' : '#7e7a9c',
                boxShadow: side === 'BUY' ? '0 4px 14px rgba(16, 185, 129, 0.4)' : 'none',
                transition: 'all 0.2s ease',
              }}
            >
              BUY (Long)
            </button>
            <button
              onClick={() => setSide('SELL')}
              style={{
                padding: '10px',
                borderRadius: '8px',
                border: 'none',
                fontWeight: 700,
                fontSize: '13px',
                cursor: 'pointer',
                backgroundColor: side === 'SELL' ? '#ef4444' : 'transparent',
                color: side === 'SELL' ? '#ffffff' : '#7e7a9c',
                boxShadow: side === 'SELL' ? '0 4px 14px rgba(239, 68, 68, 0.4)' : 'none',
                transition: 'all 0.2s ease',
              }}
            >
              SELL (Short)
            </button>
          </div>

          {/* Stock Selector */}
          <div style={{ marginBottom: '16px' }}>
            <label style={{ fontSize: '11px', fontWeight: 600, color: '#8b87a8', textTransform: 'uppercase', marginBottom: '6px', display: 'block' }}>
              Select Instrument
            </label>
            <select
              value={selectedSymbol}
              onChange={(e) => handleSymbolChange(e.target.value)}
              className="input-field"
              style={{ padding: '10px 14px', borderRadius: '10px', cursor: 'pointer' }}
            >
              {stocks.map((s) => (
                <option key={s.symbol} value={s.symbol} style={{ backgroundColor: '#161423' }}>
                  {s.symbol} — {s.company_name || s.name} ({formatPaiseToINR(s.current_price_paise || s.current_price)})
                </option>
              ))}
            </select>
          </div>

          {/* Order Type Indicator */}
          <div style={{ marginBottom: '16px' }}>
            <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '6px' }}>
              <label style={{ fontSize: '11px', fontWeight: 600, color: '#8b87a8', textTransform: 'uppercase' }}>
                Order Execution Type
              </label>
              <span className="pill-badge pill-neutral" style={{ fontSize: '10px' }}>
                LIMIT ORDER
              </span>
            </div>
            <div
              style={{
                padding: '10px 12px',
                borderRadius: '10px',
                backgroundColor: 'rgba(255, 255, 255, 0.02)',
                border: '1px solid rgba(255, 255, 255, 0.05)',
                fontSize: '11px',
                color: isMarketable ? '#34d399' : '#a78bfa',
                display: 'flex',
                alignItems: 'center',
                gap: '8px',
              }}
            >
              <Zap size={14} />
              {isMarketable
                ? `Marketable Limit: Executes immediately at ₹${(marketPricePaise / 100).toFixed(2)}`
                : 'Resting Limit: Remains pending in order book until matched'}
            </div>
          </div>

          {/* Quantity & Quick Increment Buttons */}
          <div style={{ marginBottom: '16px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '6px' }}>
              <label style={{ fontSize: '11px', fontWeight: 600, color: '#8b87a8', textTransform: 'uppercase' }}>
                Quantity (Shares)
              </label>
              <div style={{ display: 'flex', gap: '4px' }}>
                {[10, 25, 50, 100].map((inc) => (
                  <button
                    key={inc}
                    type="button"
                    onClick={() => setQuantity(inc)}
                    className="btn btn-sm btn-ghost"
                    style={{ padding: '2px 6px', fontSize: '10px' }}
                  >
                    +{inc}
                  </button>
                ))}
              </div>
            </div>
            <input
              type="number"
              min="1"
              value={quantity}
              onChange={(e) => setQuantity(Math.max(1, parseInt(e.target.value) || 1))}
              className="input-field mono"
              style={{ fontSize: '15px', fontWeight: 600 }}
            />
          </div>

          {/* Limit Price Input */}
          <div style={{ marginBottom: '20px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '6px' }}>
              <label style={{ fontSize: '11px', fontWeight: 600, color: '#8b87a8', textTransform: 'uppercase' }}>
                Limit Price per Share (₹)
              </label>
              <button
                type="button"
                onClick={handleSetMarketPrice}
                className="btn btn-sm btn-ghost"
                style={{ padding: '2px 8px', fontSize: '10px', color: '#a78bfa' }}
              >
                Match Market Price
              </button>
            </div>
            <input
              type="number"
              step="0.05"
              value={priceRupees}
              onChange={(e) => setPriceRupees(parseFloat(e.target.value) || 0)}
              className="input-field mono"
              style={{ fontSize: '15px', fontWeight: 600 }}
            />
          </div>

          {/* Pre-trade Order Summary & Validation */}
          <div
            style={{
              padding: '14px',
              backgroundColor: 'rgba(255, 255, 255, 0.02)',
              borderRadius: '12px',
              border: '1px solid rgba(255, 255, 255, 0.05)',
              marginBottom: '20px',
              display: 'flex',
              flexDirection: 'column',
              gap: '8px',
            }}
          >
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '12px' }}>
              <span style={{ color: '#8b87a8' }}>Estimated Order Value</span>
              <span className="mono" style={{ fontWeight: 700, color: '#ffffff' }}>
                {formatPaiseToINR(estimatedTotalPaise)}
              </span>
            </div>

            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '12px' }}>
              <span style={{ color: '#8b87a8' }}>Available Cash Balance</span>
              <span className="mono" style={{ color: '#c4c0e0' }}>
                {formatPaiseToINR(availableCashPaise)}
              </span>
            </div>

            {isInsufficientCash && (
              <div
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  gap: '6px',
                  color: '#f87171',
                  fontSize: '11px',
                  marginTop: '4px',
                }}
              >
                <AlertCircle size={14} /> Insufficient cash balance for this buy order.
              </div>
            )}

            {orderError && (
              <div
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  gap: '6px',
                  color: '#f87171',
                  fontSize: '12px',
                  marginTop: '4px',
                  padding: '8px',
                  backgroundColor: 'rgba(239, 68, 68, 0.1)',
                  borderRadius: '8px',
                }}
              >
                <AlertCircle size={14} /> {orderError}
              </div>
            )}
          </div>

          {/* Submit Button */}
          <button
            onClick={() => setShowConfirmModal(true)}
            disabled={!canSubmit || isLoading}
            className={`btn ${side === 'BUY' ? 'btn-success' : 'btn-danger'}`}
            style={{
              width: '100%',
              padding: '14px',
              fontSize: '14px',
              fontWeight: 700,
              opacity: canSubmit ? 1 : 0.5,
              cursor: canSubmit ? 'pointer' : 'not-allowed',
            }}
          >
            {isLoading ? 'Processing in C++ Engine...' : `${side} ${quantity} ${activeStock.symbol}`}
          </button>
        </div>

        {/* Right: Active Order Book & Recent Orders */}
        <div className="glass-card" style={{ padding: '22px 20px', display: 'flex', flexDirection: 'column', minWidth: 0 }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '16px', flexWrap: 'wrap', gap: '8px' }}>
            <div>
              <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff' }}>Order History</h3>
              <div style={{ fontSize: '11px', color: '#7e7a9c' }}>Authoritative records from SQLite `orders` table</div>
            </div>
            <span className="pill-badge pill-neutral">{orders.length} Total</span>
          </div>

          <div className="table-responsive" style={{ flex: 1 }}>
            <table className="trade-table">
              <thead>
                <tr>
                  <th>Order ID</th>
                  <th>Symbol</th>
                  <th>Side</th>
                  <th>Qty</th>
                  <th>Limit Price</th>
                  <th>Status</th>
                  <th style={{ textAlign: 'right' }}>Action</th>
                </tr>
              </thead>
              <tbody>
                {orders.map((ord) => {
                  const isBuy = ord.side === 'BUY';
                  const isPending = ord.status === 'PENDING';
                  return (
                    <tr key={ord.order_id}>
                      <td className="mono" style={{ color: '#8b87a8', fontSize: '12px' }}>
                        #{ord.order_id}
                      </td>
                      <td style={{ fontWeight: 700, color: '#ffffff' }}>{ord.symbol}</td>
                      <td>
                        <span className={`pill-badge ${isBuy ? 'pill-green' : 'pill-red'}`}>
                          {ord.side}
                        </span>
                      </td>
                      <td className="mono">{ord.quantity}</td>
                      <td className="mono">{formatPaiseToINR(ord.limit_price_paise || ord.price)}</td>
                      <td>
                        <span
                          className={`pill-badge ${
                            ord.status === 'EXECUTED'
                              ? 'pill-green'
                              : ord.status === 'CANCELLED'
                              ? 'pill-neutral'
                              : 'pill-amber'
                          }`}
                        >
                          {ord.status}
                        </span>
                      </td>
                      <td style={{ textAlign: 'right' }}>
                        {isPending && (
                          <button
                            onClick={() => cancelOrder(ord.order_id)}
                            className="btn btn-sm btn-danger"
                            style={{ fontSize: '10px', padding: '3px 8px' }}
                          >
                            Cancel
                          </button>
                        )}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      </div>

      {/* Confirmation Modal */}
      {showConfirmModal && (
        <div className="modal-overlay">
          <div className="modal-content" style={{ padding: '24px' }}>
            <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '16px' }}>
              <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff' }}>Confirm Order</h3>
              <button
                onClick={() => setShowConfirmModal(false)}
                style={{ background: 'none', border: 'none', color: '#8b87a8', cursor: 'pointer' }}
              >
                <X size={18} />
              </button>
            </div>

            <p style={{ fontSize: '13px', color: '#8b87a8', marginBottom: '20px' }}>
              Order will be transmitted to the C++ TradingEngine for validation and execution.
            </p>

            <div
              style={{
                backgroundColor: 'rgba(255, 255, 255, 0.03)',
                padding: '16px',
                borderRadius: '12px',
                display: 'flex',
                flexDirection: 'column',
                gap: '10px',
                marginBottom: '20px',
              }}
            >
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Side / Action</span>
                <span className={`pill-badge ${side === 'BUY' ? 'pill-green' : 'pill-red'}`}>{side}</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Stock</span>
                <span style={{ fontWeight: 700, color: '#ffffff' }}>{activeStock.symbol}</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Quantity</span>
                <span className="mono" style={{ fontWeight: 700, color: '#ffffff' }}>{quantity} shares</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Limit Price</span>
                <span className="mono" style={{ fontWeight: 700, color: '#ffffff' }}>{formatPaiseToINR(pricePaise)}</span>
              </div>
              <div style={{ height: '1px', backgroundColor: 'rgba(255,255,255,0.06)', margin: '4px 0' }} />
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '14px', fontWeight: 700 }}>
                <span style={{ color: '#c4c0e0' }}>Total Value</span>
                <span className="mono" style={{ color: '#34d399' }}>{formatPaiseToINR(estimatedTotalPaise)}</span>
              </div>
            </div>

            <div style={{ display: 'flex', gap: '10px' }}>
              <button
                onClick={() => setShowConfirmModal(false)}
                className="btn btn-ghost"
                style={{ flex: 1 }}
              >
                Cancel
              </button>
              <button
                onClick={handleOrderSubmit}
                className="btn btn-primary"
                style={{ flex: 1 }}
              >
                Transmit Order
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
