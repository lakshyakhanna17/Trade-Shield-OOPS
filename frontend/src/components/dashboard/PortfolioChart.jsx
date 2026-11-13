/**
 * TradeShield Portfolio Performance Area Chart
 * Displays overall portfolio trajectory with key financial summaries.
 */

import { AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer } from 'recharts';
import { ArrowUpRight, ArrowDownRight, Activity } from 'lucide-react';
import { formatPaiseToINR, formatRupees, formatPercent } from '../../utils/formatters';

export default function PortfolioChart({ portfolio }) {
  const isPositive = portfolio.total_unrealized_pnl >= 0;
  const snapshots = Array.isArray(portfolio.performance_history) ? portfolio.performance_history : [];
  const hasHistory = snapshots.length >= 2;

  return (
    <div className="glass-card" style={{ padding: '22px 20px 18px', height: '100%', minWidth: 0 }}>
      {/* Top Header */}
      <div
        style={{
          display: 'flex',
          alignItems: 'flex-start',
          justifyContent: 'space-between',
          marginBottom: '16px',
          flexWrap: 'wrap',
          gap: '14px',
        }}
      >
        <div style={{ minWidth: 0 }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '8px', flexWrap: 'wrap' }}>
            <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', letterSpacing: '0.05em', fontWeight: 600 }}>
              Account Valuation
            </div>
            <span
              className={`pill-badge ${hasHistory ? 'pill-purple' : 'pill-neutral'}`}
              style={{ fontSize: '10px', letterSpacing: '0.04em', padding: '2px 6px' }}
            >
              {hasHistory ? `SQLite History (${snapshots.length} snapshots)` : 'Live Valuation'}
            </span>
          </div>
          <div style={{ fontSize: 'clamp(22px, 4vw, 28px)', fontWeight: 800, color: '#ffffff', letterSpacing: '-0.03em', marginTop: '4px' }}>
            {formatPaiseToINR(portfolio.total_account_value)}
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '8px', marginTop: '4px', flexWrap: 'wrap' }}>
            <span
              className={`pill-badge ${isPositive ? 'pill-green' : 'pill-red'}`}
              style={{ fontSize: '11px', padding: '3px 8px' }}
            >
              {isPositive ? <ArrowUpRight size={12} /> : <ArrowDownRight size={12} />}
              {formatPaiseToINR(portfolio.total_unrealized_pnl)} ({formatPercent(portfolio.total_pnl_percent)})
            </span>
            <span style={{ fontSize: '11px', color: '#7a7698' }}>Total Unrealized P&L</span>
          </div>
        </div>

        {/* Breakdown Chips */}
        <div style={{ display: 'flex', gap: '10px', flexWrap: 'wrap' }}>
          <div
            style={{
              backgroundColor: 'rgba(255, 255, 255, 0.03)',
              padding: '6px 12px',
              borderRadius: '10px',
              border: '1px solid rgba(255, 255, 255, 0.05)',
              textAlign: 'right',
              flex: '1 1 auto',
            }}
          >
            <div style={{ fontSize: '10px', color: '#7a7698', textTransform: 'uppercase' }}>Cash Balance</div>
            <div className="mono" style={{ fontSize: '13px', fontWeight: 700, color: '#ffffff' }}>
              {formatPaiseToINR(portfolio.available_cash, false)}
            </div>
          </div>
          <div
            style={{
              backgroundColor: 'rgba(255, 255, 255, 0.03)',
              padding: '6px 12px',
              borderRadius: '10px',
              border: '1px solid rgba(255, 255, 255, 0.05)',
              textAlign: 'right',
              flex: '1 1 auto',
            }}
          >
            <div style={{ fontSize: '10px', color: '#7a7698', textTransform: 'uppercase' }}>Invested Holdings</div>
            <div className="mono" style={{ fontSize: '13px', fontWeight: 700, color: '#a78bfa' }}>
              {formatPaiseToINR(portfolio.invested_value, false)}
            </div>
          </div>
        </div>
      </div>

      {/* Chart Area or Empty State */}
      <div style={{ width: '100%', height: '180px', marginTop: '10px', minWidth: 0 }}>
        {hasHistory ? (
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart
              data={snapshots}
              margin={{ top: 10, right: 10, left: -20, bottom: 0 }}
            >
              <defs>
                <linearGradient id="portfolioGrad" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#7c3aed" stopOpacity={0.4} />
                  <stop offset="100%" stopColor="#7c3aed" stopOpacity={0.0} />
                </linearGradient>
              </defs>
              <XAxis
                dataKey="date"
                stroke="#524e6e"
                fontSize={11}
                tickLine={false}
                axisLine={{ stroke: 'rgba(255, 255, 255, 0.06)' }}
              />
              <YAxis
                stroke="#524e6e"
                fontSize={11}
                tickLine={false}
                domain={[(min) => Math.floor(min * 0.98), (max) => Math.ceil(max * 1.02)]}
                axisLine={{ stroke: 'rgba(255, 255, 255, 0.06)' }}
                tickFormatter={(val) => `₹${(val / 100000).toFixed(1)}L`}
              />
              <Tooltip
                content={({ active, payload, label }) => {
                  if (active && payload && payload.length) {
                    const pt = payload[0].payload;
                    return (
                      <div
                        style={{
                          backgroundColor: '#161423',
                          border: '1px solid rgba(255, 255, 255, 0.12)',
                          padding: '8px 12px',
                          borderRadius: '10px',
                          fontSize: '12px',
                          boxShadow: '0 8px 24px rgba(0,0,0,0.5)',
                        }}
                      >
                        <div style={{ color: '#8b87a8', fontSize: '10px', marginBottom: '4px' }}>Recorded: {label}</div>
                        <div style={{ color: '#ffffff', fontWeight: 700 }}>
                          Valuation: {formatRupees(payload[0].value)}
                        </div>
                        {pt?.cash !== undefined && (
                          <div style={{ color: '#a78bfa', fontSize: '11px', marginTop: '2px' }}>
                            Cash: {formatRupees(pt.cash)}
                          </div>
                        )}
                        {pt?.holdings !== undefined && (
                          <div style={{ color: '#34d399', fontSize: '11px', marginTop: '2px' }}>
                            Holdings: {formatRupees(pt.holdings)}
                          </div>
                        )}
                      </div>
                    );
                  }
                  return null;
                }}
              />
              <Area
                type="monotone"
                dataKey="portfolioValue"
                stroke="#8b5cf6"
                strokeWidth={2.5}
                fillOpacity={1}
                fill="url(#portfolioGrad)"
                activeDot={{ r: 5, fill: '#ffffff', stroke: '#8b5cf6', strokeWidth: 2 }}
              />
            </AreaChart>
          </ResponsiveContainer>
        ) : (
          <div
            style={{
              height: '100%',
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              justifyContent: 'center',
              backgroundColor: 'rgba(255, 255, 255, 0.02)',
              borderRadius: '12px',
              border: '1px dashed rgba(255, 255, 255, 0.08)',
              padding: '16px 24px',
              textAlign: 'center',
            }}
          >
            <Activity size={24} color="#7c3aed" style={{ marginBottom: '8px', opacity: 0.8 }} />
            <div style={{ fontSize: '13px', fontWeight: 600, color: '#e2e0f0' }}>
              Insufficient Snapshot History
            </div>
            <div style={{ fontSize: '11px', color: '#7e7a9c', maxWidth: '380px', marginTop: '4px', lineHeight: 1.4 }}>
              {snapshots.length === 1
                ? `1 initial baseline snapshot recorded at ${snapshots[0].date || snapshots[0].timestamp}. Additional snapshots will record automatically upon trade executions.`
                : 'Real portfolio snapshots are persisted in SQLite upon trade executions. Place orders or run the simulation engine to generate trajectory history.'}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
