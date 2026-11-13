/**
 * TradeShield Portfolio & Holdings Page
 * Asset breakdown, profit & loss analysis, and portfolio allocation.
 */

import { PieChart, Pie, Cell, ResponsiveContainer, Tooltip } from 'recharts';
import { ArrowUpRight, ArrowDownRight, Sliders } from 'lucide-react';
import { useTradeShield } from '../context/useTradeShield';
import { formatPaiseToINR, formatPercent } from '../utils/formatters';
import { useNavigate } from 'react-router-dom';

const ALLOCATION_COLORS = ['#F59E0B', '#3B82F6', '#8B5CF6', '#10B981', '#EC4899', '#06B6D4'];

export default function Portfolio() {
  const navigate = useNavigate();
  const { portfolio, holdings } = useTradeShield();

  const isTotalPnlPositive = portfolio.total_unrealized_pnl >= 0;

  // Chart data for stock allocation
  const totalHoldingsValue = holdings.reduce((sum, h) => sum + (h.current_value || 0), 0) / 100;
  const pieData = holdings.map((h, i) => {
    const valInr = (h.current_value || 0) / 100;
    const pct = totalHoldingsValue > 0 ? (valInr / totalHoldingsValue) * 100 : 0;
    return {
      name: h.symbol,
      value: valInr,
      percent: pct,
      color: ALLOCATION_COLORS[i % ALLOCATION_COLORS.length],
    };
  });

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Header */}
      <div>
        <h1 style={{ fontSize: '28px', fontWeight: 800, color: '#ffffff', letterSpacing: '-0.02em' }}>
          Portfolio & Asset Allocation
        </h1>
        <p style={{ fontSize: '13px', color: '#8b87a8', marginTop: '2px' }}>
          Real-time mark-to-market valuations and equity positions managed by TradeShield.
        </p>
      </div>

      {/* Top 4 Metric Cards */}
      <div
        style={{
          display: 'grid',
          gridTemplateColumns: 'repeat(auto-fit, minmax(240px, 1fr))',
          gap: '16px',
        }}
      >
        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
            Total Account Value
          </div>
          <div className="mono" style={{ fontSize: '26px', fontWeight: 800, color: '#ffffff', marginTop: '6px' }}>
            {formatPaiseToINR(portfolio.total_account_value)}
          </div>
          <div style={{ fontSize: '11px', color: '#8b87a8', marginTop: '4px' }}>
            Cash + Live Holdings
          </div>
        </div>

        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
            Invested Value
          </div>
          <div className="mono" style={{ fontSize: '26px', fontWeight: 800, color: '#a78bfa', marginTop: '6px' }}>
            {formatPaiseToINR(portfolio.invested_value)}
          </div>
          <div style={{ fontSize: '11px', color: '#8b87a8', marginTop: '4px' }}>
            {holdings.length} Active Positions
          </div>
        </div>

        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
            Available Cash
          </div>
          <div className="mono" style={{ fontSize: '26px', fontWeight: 800, color: '#ffffff', marginTop: '6px' }}>
            {formatPaiseToINR(portfolio.available_cash)}
          </div>
          <div style={{ fontSize: '11px', color: '#10b981', marginTop: '4px' }}>
            Ready for execution
          </div>
        </div>

        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
            Total Unrealized P&L
          </div>
          <div
            className="mono"
            style={{
              fontSize: '26px',
              fontWeight: 800,
              color: isTotalPnlPositive ? '#34d399' : '#f87171',
              marginTop: '6px',
            }}
          >
            {formatPaiseToINR(portfolio.total_unrealized_pnl)}
          </div>
          <div
            style={{
              fontSize: '11px',
              fontWeight: 600,
              color: isTotalPnlPositive ? '#34d399' : '#f87171',
              marginTop: '4px',
              display: 'flex',
              alignItems: 'center',
              gap: '2px',
            }}
          >
            {isTotalPnlPositive ? <ArrowUpRight size={14} /> : <ArrowDownRight size={14} />}
            {formatPercent(portfolio.total_pnl_percent)} overall return
          </div>
        </div>
      </div>

      {/* Middle Row: Allocation Donut Chart + Breakdown Legend */}
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 2fr', gap: '20px' }}>
        <div className="glass-card" style={{ padding: '24px', display: 'flex', flexDirection: 'column' }}>
          <div style={{ fontSize: '16px', fontWeight: 700, color: '#ffffff', marginBottom: '8px' }}>
            Holdings Allocation
          </div>
          {pieData.length === 0 ? (
            <div style={{ height: '220px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#7e7a9c', fontSize: '12px', textAlign: 'center' }}>
              No equity positions held. Portfolio is 100% in cash.
            </div>
          ) : (
            <>
              <div style={{ width: '100%', height: '220px' }}>
                <ResponsiveContainer width="100%" height="100%">
                  <PieChart>
                    <Pie
                      data={pieData}
                      cx="50%"
                      cy="50%"
                      innerRadius={60}
                      outerRadius={85}
                      paddingAngle={5}
                      dataKey="value"
                    >
                      {pieData.map((entry, index) => (
                        <Cell key={`cell-${index}`} fill={entry.color} stroke="rgba(0,0,0,0.5)" />
                      ))}
                    </Pie>
                    <Tooltip
                      formatter={(value, name, item) => [
                        `₹${Number(value).toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })} (${item.payload.percent.toFixed(1)}%)`,
                        'Holdings Valuation',
                      ]}
                      contentStyle={{
                        backgroundColor: '#161423',
                        border: '1px solid rgba(255,255,255,0.1)',
                        borderRadius: '8px',
                      }}
                    />
                  </PieChart>
                </ResponsiveContainer>
              </div>

              <div style={{ display: 'flex', flexWrap: 'wrap', gap: '10px', justifyContent: 'center' }}>
                {pieData.map((d) => (
                  <div key={d.name} style={{ display: 'flex', alignItems: 'center', gap: '6px', fontSize: '11px' }}>
                    <span style={{ width: '8px', height: '8px', borderRadius: '50%', backgroundColor: d.color }} />
                    <span style={{ color: '#c4c0e0', fontWeight: 600 }}>{d.name}</span>
                    <span style={{ color: '#7e7a9c', fontSize: '10px' }}>({d.percent.toFixed(1)}%)</span>
                  </div>
                ))}
              </div>
            </>
          )}
        </div>

        {/* Detailed Positions Table */}
        <div className="glass-card" style={{ padding: '24px' }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '16px' }}>
            <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff' }}>Stock Holdings</h3>
            <span className="pill-badge pill-neutral">{holdings.length} Positions</span>
          </div>

          <div style={{ overflowX: 'auto' }}>
            <table className="trade-table">
              <thead>
                <tr>
                  <th>Stock</th>
                  <th>Quantity</th>
                  <th>Avg Buy Price</th>
                  <th>Current Price</th>
                  <th>Current Value</th>
                  <th>P&L (₹)</th>
                  <th>P&L (%)</th>
                  <th style={{ textAlign: 'right' }}>Action</th>
                </tr>
              </thead>
              <tbody>
                {holdings.length === 0 ? (
                  <tr>
                    <td colSpan="8" style={{ textAlign: 'center', padding: '32px', color: '#7e7a9c', fontSize: '13px' }}>
                      No active equity positions in portfolio. Place a BUY order on the Trading page to build holdings.
                    </td>
                  </tr>
                ) : (
                  holdings.map((h) => {
                    const pnlUp = h.unrealized_pnl >= 0;
                    return (
                      <tr key={h.symbol}>
                        <td>
                          <div style={{ fontWeight: 700, color: '#ffffff' }}>{h.symbol}</div>
                          <div style={{ fontSize: '11px', color: '#7e7a9c' }}>{h.name}</div>
                        </td>
                        <td className="mono" style={{ fontWeight: 600 }}>{h.quantity}</td>
                        <td className="mono">{formatPaiseToINR(h.average_buy_price)}</td>
                        <td className="mono" style={{ color: '#ffffff', fontWeight: 600 }}>
                          {formatPaiseToINR(h.current_price)}
                        </td>
                        <td className="mono" style={{ color: '#a78bfa', fontWeight: 700 }}>
                          {formatPaiseToINR(h.current_value)}
                        </td>
                        <td className="mono" style={{ color: pnlUp ? '#34d399' : '#f87171', fontWeight: 700 }}>
                          {formatPaiseToINR(h.unrealized_pnl)}
                        </td>
                        <td>
                          <span className={`pill-badge ${pnlUp ? 'pill-green' : 'pill-red'}`}>
                            {formatPercent(h.pnl_percent)}
                          </span>
                        </td>
                        <td style={{ textAlign: 'right' }}>
                          <button
                            onClick={() => navigate('/trading', { state: { preselectStock: h.symbol } })}
                            className="btn btn-sm btn-ghost"
                            style={{ fontSize: '11px', gap: '4px' }}
                          >
                            <Sliders size={12} /> Trade
                          </button>
                        </td>
                      </tr>
                    );
                  })
                )}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
}
