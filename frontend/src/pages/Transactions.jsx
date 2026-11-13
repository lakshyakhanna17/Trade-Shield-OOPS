/**
 * TradeShield Transactions & Order Audit Page
 * Full historical ledger of submitted orders and executed trades.
 */

import { useState } from 'react';
import { Search } from 'lucide-react';
import { useTradeShield } from '../context/useTradeShield';
import { formatPaiseToINR, formatDateTime, formatRelativeTime } from '../utils/formatters';

export default function Transactions() {
  const { orders, trades, cancelOrder } = useTradeShield();
  const [activeTab, setActiveTab] = useState('ORDERS');
  const [search, setSearch] = useState('');
  const [sideFilter, setSideFilter] = useState('ALL');
  const [statusFilter, setStatusFilter] = useState('ALL');

  // Filtered orders
  const filteredOrders = orders.filter((ord) => {
    const matchesSearch =
      ord.symbol.toLowerCase().includes(search.toLowerCase()) ||
      String(ord.order_id).includes(search) ||
      (ord.trader_name && ord.trader_name.toLowerCase().includes(search.toLowerCase()));
    if (!matchesSearch) return false;
    if (sideFilter !== 'ALL' && ord.side !== sideFilter) return false;
    if (statusFilter !== 'ALL' && ord.status !== statusFilter) return false;
    return true;
  });

  // Filtered trades
  const filteredTrades = trades.filter((trd) => {
    return (
      trd.symbol.toLowerCase().includes(search.toLowerCase()) ||
      String(trd.trade_id).includes(search) ||
      (trd.buyer_name && trd.buyer_name.toLowerCase().includes(search.toLowerCase())) ||
      (trd.seller_name && trd.seller_name.toLowerCase().includes(search.toLowerCase()))
    );
  });

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Header */}
      <div>
        <h1 style={{ fontSize: '28px', fontWeight: 800, color: '#ffffff', letterSpacing: '-0.02em' }}>
          Transactions & Order Audit
        </h1>
        <p style={{ fontSize: '13px', color: '#8b87a8', marginTop: '2px' }}>
          Immutable audit record of all client orders and matched trades executed by the trading engine.
        </p>
      </div>

      {/* Main Glass Card */}
      <div className="glass-card" style={{ padding: '24px' }}>
        {/* Top Controls: Tabs & Search Filters */}
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            flexWrap: 'wrap',
            gap: '16px',
            marginBottom: '20px',
          }}
        >
          {/* Tab Switcher */}
          <div style={{ display: 'flex', gap: '6px' }}>
            <button
              onClick={() => setActiveTab('ORDERS')}
              className="btn btn-sm"
              style={{
                backgroundColor: activeTab === 'ORDERS' ? 'rgba(255, 255, 255, 0.1)' : 'transparent',
                color: activeTab === 'ORDERS' ? '#ffffff' : '#7e7a9c',
                border: activeTab === 'ORDERS' ? '1px solid rgba(255, 255, 255, 0.16)' : '1px solid transparent',
                padding: '8px 16px',
                fontSize: '13px',
              }}
            >
              All Orders ({orders.length})
            </button>
            <button
              onClick={() => setActiveTab('TRADES')}
              className="btn btn-sm"
              style={{
                backgroundColor: activeTab === 'TRADES' ? 'rgba(255, 255, 255, 0.1)' : 'transparent',
                color: activeTab === 'TRADES' ? '#ffffff' : '#7e7a9c',
                border: activeTab === 'TRADES' ? '1px solid rgba(255, 255, 255, 0.16)' : '1px solid transparent',
                padding: '8px 16px',
                fontSize: '13px',
              }}
            >
              Executed Trades ({trades.length})
            </button>
          </div>

          {/* Search & Filters */}
          <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
            <div style={{ position: 'relative', width: '220px' }}>
              <Search size={14} color="#7e7a9c" style={{ position: 'absolute', left: '10px', top: '11px' }} />
              <input
                type="text"
                placeholder="Search symbol, ID, trader..."
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                className="input-field"
                style={{ paddingLeft: '32px', padding: '8px 12px 8px 32px', fontSize: '12px' }}
              />
            </div>

            {activeTab === 'ORDERS' && (
              <>
                <select
                  value={sideFilter}
                  onChange={(e) => setSideFilter(e.target.value)}
                  className="input-field"
                  style={{ width: 'auto', padding: '6px 12px', fontSize: '12px' }}
                >
                  <option value="ALL">All Sides</option>
                  <option value="BUY">BUY Only</option>
                  <option value="SELL">SELL Only</option>
                </select>

                <select
                  value={statusFilter}
                  onChange={(e) => setStatusFilter(e.target.value)}
                  className="input-field"
                  style={{ width: 'auto', padding: '6px 12px', fontSize: '12px' }}
                >
                  <option value="ALL">All Statuses</option>
                  <option value="PENDING">PENDING</option>
                  <option value="EXECUTED">EXECUTED</option>
                  <option value="CANCELLED">CANCELLED</option>
                </select>
              </>
            )}
          </div>
        </div>

        {/* Tab 1: Orders Table */}
        {activeTab === 'ORDERS' && (
          <div style={{ overflowX: 'auto' }}>
            <table className="trade-table">
              <thead>
                <tr>
                  <th>Order ID</th>
                  <th>Trader</th>
                  <th>Symbol</th>
                  <th>Side</th>
                  <th>Type</th>
                  <th>Qty</th>
                  <th>Price</th>
                  <th>Total Value</th>
                  <th>Status</th>
                  <th>Time</th>
                  <th style={{ textAlign: 'right' }}>Action</th>
                </tr>
              </thead>
              <tbody>
                {filteredOrders.length === 0 ? (
                  <tr>
                    <td colSpan="11" style={{ textAlign: 'center', padding: '32px', color: '#7e7a9c', fontSize: '13px' }}>
                      No orders match your filter criteria.
                    </td>
                  </tr>
                ) : (
                  filteredOrders.map((ord) => {
                    const isBuy = ord.side === 'BUY';
                    const isPending = ord.status === 'PENDING';
                    const totalPaise = ord.quantity * ord.price;
                    return (
                      <tr key={ord.order_id}>
                        <td className="mono" style={{ color: '#8b87a8' }}>#{ord.order_id}</td>
                        <td>
                          <div style={{ fontWeight: 600, color: '#ffffff' }}>{ord.trader_name || `Trader #${ord.trader_id}`}</div>
                        </td>
                        <td style={{ fontWeight: 700, color: '#a78bfa' }}>{ord.symbol}</td>
                        <td>
                          <span className={`pill-badge ${isBuy ? 'pill-green' : 'pill-red'}`}>
                            {ord.side}
                          </span>
                        </td>
                        <td style={{ fontSize: '11px', color: '#7e7a9c' }}>{ord.type}</td>
                        <td className="mono">{ord.quantity}</td>
                        <td className="mono">{formatPaiseToINR(ord.price)}</td>
                        <td className="mono" style={{ color: '#ffffff', fontWeight: 600 }}>
                          {formatPaiseToINR(totalPaise)}
                        </td>
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
                        <td style={{ fontSize: '11px', color: '#7e7a9c' }}>
                          {formatRelativeTime(ord.created_at)}
                        </td>
                        <td style={{ textAlign: 'right' }}>
                          {isPending && (
                            <button
                              onClick={() => cancelOrder(ord.order_id)}
                              className="btn btn-sm btn-danger"
                              style={{ fontSize: '11px', padding: '3px 8px' }}
                            >
                              Cancel
                            </button>
                          )}
                        </td>
                      </tr>
                    );
                  })
                )}
              </tbody>
            </table>
          </div>
        )}

        {/* Tab 2: Executed Trades Table */}
        {activeTab === 'TRADES' && (
          <div style={{ overflowX: 'auto' }}>
            <table className="trade-table">
              <thead>
                <tr>
                  <th>Trade ID</th>
                  <th>Stock</th>
                  <th>Buyer</th>
                  <th>Seller</th>
                  <th>Quantity</th>
                  <th>Execution Price</th>
                  <th>Trade Value</th>
                  <th>Executed At</th>
                </tr>
              </thead>
              <tbody>
                {filteredTrades.length === 0 ? (
                  <tr>
                    <td colSpan="8" style={{ textAlign: 'center', padding: '32px', color: '#7e7a9c', fontSize: '13px' }}>
                      No trade execution records found.
                    </td>
                  </tr>
                ) : (
                  filteredTrades.map((trd) => (
                    <tr key={trd.trade_id}>
                      <td className="mono" style={{ color: '#8b87a8' }}>#{trd.trade_id}</td>
                      <td style={{ fontWeight: 700, color: '#ffffff' }}>{trd.symbol}</td>
                      <td style={{ color: '#34d399', fontWeight: 600 }}>{trd.buyer_name || `Trader #${trd.buyer_id}`}</td>
                      <td style={{ color: '#f87171', fontWeight: 600 }}>{trd.seller_name || `Trader #${trd.seller_id}`}</td>
                      <td className="mono">{trd.quantity}</td>
                      <td className="mono" style={{ fontWeight: 600, color: '#ffffff' }}>
                        {formatPaiseToINR(trd.execution_price)}
                      </td>
                      <td className="mono" style={{ color: '#a78bfa', fontWeight: 700 }}>
                        {formatPaiseToINR(trd.trade_value)}
                      </td>
                      <td style={{ fontSize: '11px', color: '#7e7a9c' }}>
                        {formatDateTime(trd.executed_at)}
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  );
}
