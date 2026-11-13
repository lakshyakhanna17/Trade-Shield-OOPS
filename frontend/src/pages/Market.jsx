/**
 * TradeShield Market Overview Page
 * Full market intelligence with real-time stock quotes, interactive charts, and filtering.
 */

import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  Search,
  Sliders,
  ArrowUpRight,
  ArrowDownRight,
} from 'lucide-react';
import { AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer } from 'recharts';
import { useTradeShield } from '../context/useTradeShield';
import {
  formatPaiseToINR,
  formatPercent,
  formatNumberIN,
} from '../utils/formatters';

export default function Market() {
  const navigate = useNavigate();
  const { stocks } = useTradeShield();
  const [selectedSymbol, setSelectedSymbol] = useState(stocks[0]?.symbol || 'RELIANCE');
  const selectedStock = stocks.find((s) => s.symbol === selectedSymbol) || stocks[0] || null;
  const [search, setSearch] = useState('');
  const [timeframe, setTimeframe] = useState('1D');
  const [filterType, setFilterType] = useState('ALL');

  const filteredStocks = stocks.filter((stock) => {
    const matchesSearch =
      stock.symbol.toLowerCase().includes(search.toLowerCase()) ||
      stock.name.toLowerCase().includes(search.toLowerCase());
    if (!matchesSearch) return false;
    if (filterType === 'GAINERS') return stock.change_24h > 0;
    if (filterType === 'LOSERS') return stock.change_24h < 0;
    if (filterType === 'HIGH_VOL') return stock.volume > 150000;
    return true;
  });

  const isUp = selectedStock?.change_24h >= 0;

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Header */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: '16px' }}>
        <div>
          <h1 className="page-title">
            NSE Market Intelligence
          </h1>
          <p className="page-subtitle">
            Real-time simulated stock prices and trading volumes on the National Stock Exchange of India.
          </p>
        </div>

        {/* Filter / Search Bar */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px', flexWrap: 'wrap', width: 'auto' }}>
          <div style={{ position: 'relative', width: '220px', minWidth: '170px', flex: '1 1 auto' }}>
            <Search size={14} color="#7e7a9c" style={{ position: 'absolute', left: '12px', top: '12px' }} />
            <input
              type="text"
              placeholder="Search symbols..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              className="input-field"
              style={{ paddingLeft: '34px', borderRadius: '10px' }}
            />
          </div>

          <div style={{ display: 'flex', gap: '4px', overflowX: 'auto', paddingBottom: '2px', maxWidth: '100%' }}>
            {['ALL', 'GAINERS', 'LOSERS', 'HIGH_VOL'].map((f) => (
              <button
                key={f}
                onClick={() => setFilterType(f)}
                className="btn btn-sm btn-ghost"
                style={{
                  backgroundColor: filterType === f ? 'rgba(255, 255, 255, 0.1)' : 'transparent',
                  color: filterType === f ? '#ffffff' : '#7e7a9c',
                  border: filterType === f ? '1px solid rgba(255,255,255,0.15)' : '1px solid transparent',
                  whiteSpace: 'nowrap',
                  padding: '6px 12px',
                  fontSize: '12px',
                }}
              >
                {f === 'HIGH_VOL' ? 'High Volume' : f}
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* Main Market Grid: Selected Stock Chart + All Stocks List */}
      <div className="responsive-grid-2col">
        {/* Selected Stock Detailed Chart */}
        {selectedStock ? (
          <div className="glass-card" style={{ padding: '22px 20px', display: 'flex', flexDirection: 'column', minWidth: 0 }}>
            {/* Stock Title Row */}
            <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', marginBottom: '18px', flexWrap: 'wrap', gap: '12px' }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: '14px' }}>
                <div
                  style={{
                    width: '46px',
                    height: '46px',
                    borderRadius: '14px',
                    background: `linear-gradient(135deg, ${selectedStock.accent_color || '#8B5CF6'} 0%, rgba(20, 18, 30, 0.9) 100%)`,
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    fontWeight: 800,
                    fontSize: '15px',
                    color: '#ffffff',
                    boxShadow: `0 6px 18px ${selectedStock.accent_color || '#8B5CF6'}44`,
                    flexShrink: 0,
                  }}
                >
                  {selectedStock.symbol.slice(0, 2)}
                </div>
                <div>
                  <div style={{ display: 'flex', alignItems: 'center', gap: '8px', flexWrap: 'wrap' }}>
                    <h2 style={{ fontSize: '20px', fontWeight: 800, color: '#ffffff' }}>
                      {selectedStock.symbol}
                    </h2>
                    <span className="pill-badge pill-neutral">NSE: EQUITIES</span>
                  </div>
                  <div style={{ fontSize: '13px', color: '#8b87a8' }}>{selectedStock.name}</div>
                </div>
              </div>

              {/* Trade Action Button */}
              <button
                onClick={() => navigate('/trading', { state: { preselectStock: selectedStock.symbol } })}
                className="btn btn-primary"
                style={{ padding: '8px 16px', gap: '6px' }}
              >
                <Sliders size={15} /> Trade {selectedStock.symbol}
              </button>
            </div>

            {/* Price & Metrics Bar */}
            <div
              style={{
                display: 'grid',
                gridTemplateColumns: 'repeat(auto-fit, minmax(125px, 1fr))',
                gap: '12px',
                padding: '14px',
                backgroundColor: 'rgba(255, 255, 255, 0.02)',
                borderRadius: '14px',
                border: '1px solid rgba(255, 255, 255, 0.05)',
                marginBottom: '18px',
              }}
            >
              <div>
                <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>Last Price</div>
                <div className="mono" style={{ fontSize: '20px', fontWeight: 800, color: '#ffffff' }}>
                  {formatPaiseToINR(selectedStock.current_price)}
                </div>
                <div
                  style={{
                    fontSize: '11px',
                    fontWeight: 600,
                    color: isUp ? '#34d399' : '#f87171',
                    display: 'flex',
                    alignItems: 'center',
                    gap: '2px',
                    marginTop: '2px',
                  }}
                >
                  {isUp ? <ArrowUpRight size={12} /> : <ArrowDownRight size={12} />}
                  {formatPercent(selectedStock.change_24h)}
                </div>
              </div>

              <div>
                <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>24h High</div>
                <div className="mono" style={{ fontSize: '15px', fontWeight: 700, color: '#e2e0f0', marginTop: '2px' }}>
                  {formatPaiseToINR(selectedStock.high_24h_paise ?? selectedStock.high_24h ?? selectedStock.current_price)}
                </div>
              </div>

              <div>
                <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>24h Low</div>
                <div className="mono" style={{ fontSize: '15px', fontWeight: 700, color: '#e2e0f0', marginTop: '2px' }}>
                  {formatPaiseToINR(selectedStock.low_24h_paise ?? selectedStock.low_24h ?? selectedStock.current_price)}
                </div>
              </div>

              <div>
                <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>Total Volume</div>
                <div className="mono" style={{ fontSize: '15px', fontWeight: 700, color: '#a78bfa', marginTop: '2px' }}>
                  {formatNumberIN(selectedStock.volume)}
                </div>
              </div>
            </div>

            {/* Timeframe Selector & Chart */}
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px', flexWrap: 'wrap', gap: '8px' }}>
              <span className="pill-badge pill-neutral" style={{ fontSize: '10px', letterSpacing: '0.04em' }}>
                Simulated Intraday Trajectory (1D)
              </span>
              <div style={{ display: 'flex', gap: '6px', alignItems: 'center' }}>
                {['1D', '1W', '1M', '1Y'].map((t) => {
                  const isAvailable = t === '1D';
                  return (
                    <button
                      key={t}
                      disabled={!isAvailable}
                      onClick={() => isAvailable && setTimeframe(t)}
                      title={isAvailable ? 'Intraday 1D View' : `${t} multi-day history unavailable in simulated mode`}
                      style={{
                        padding: '4px 10px',
                        borderRadius: '6px',
                        fontSize: '11px',
                        fontWeight: 600,
                        backgroundColor: timeframe === t ? 'rgba(255, 255, 255, 0.12)' : 'transparent',
                        color: timeframe === t ? '#ffffff' : isAvailable ? '#7e7a9c' : '#4a4666',
                        border: 'none',
                        cursor: isAvailable ? 'pointer' : 'not-allowed',
                        opacity: isAvailable ? 1 : 0.45,
                        transition: 'all 0.15s ease',
                      }}
                    >
                      {t}
                    </button>
                  );
                })}
              </div>
            </div>

            <div style={{ width: '100%', height: '240px', minWidth: 0 }}>
              <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={selectedStock.history || []} margin={{ top: 10, right: 10, left: 0, bottom: 0 }}>
                  <defs>
                    <linearGradient id="selectedStockGrad" x1="0" y1="0" x2="0" y2="1">
                      <stop offset="0%" stopColor={selectedStock.accent_color || '#8B5CF6'} stopOpacity={0.4} />
                      <stop offset="100%" stopColor={selectedStock.accent_color || '#8B5CF6'} stopOpacity={0.0} />
                    </linearGradient>
                  </defs>
                  <XAxis dataKey="time" stroke="#524e6e" fontSize={11} tickLine={false} />
                  <YAxis
                    domain={[(dataMin) => Math.floor(dataMin * 0.995), (dataMax) => Math.ceil(dataMax * 1.005)]}
                    stroke="#524e6e"
                    fontSize={11}
                    tickLine={false}
                    orientation="right"
                    tickFormatter={(val) => `₹${Math.round(val)}`}
                  />
                  <Tooltip
                    content={({ active, payload, label }) => {
                      if (active && payload && payload.length) {
                        return (
                          <div
                            style={{
                              backgroundColor: '#161423',
                              border: '1px solid rgba(255, 255, 255, 0.12)',
                              padding: '8px 12px',
                              borderRadius: '8px',
                              fontSize: '12px',
                              color: '#ffffff',
                              boxShadow: '0 8px 24px rgba(0,0,0,0.5)',
                            }}
                          >
                            <div style={{ color: '#8b87a8', fontSize: '10px', marginBottom: '2px' }}>Time: {label}</div>
                            <div style={{ fontWeight: 700, color: selectedStock.accent_color || '#a78bfa' }}>
                              ₹{Number(payload[0].value).toFixed(2)}
                            </div>
                          </div>
                        );
                      }
                      return null;
                    }}
                  />
                  <Area
                    type="monotone"
                    dataKey="price"
                    stroke={selectedStock.accent_color || '#8B5CF6'}
                    strokeWidth={2.5}
                    fillOpacity={1}
                    fill="url(#selectedStockGrad)"
                  />
                </AreaChart>
              </ResponsiveContainer>
            </div>
          </div>
        ) : (
          <div className="glass-card" style={{ padding: '60px 24px', textAlign: 'center', color: '#8b87a8' }}>
            Loading market instruments...
          </div>
        )}

        {/* Stocks List on the Right */}
        <div className="glass-card" style={{ padding: '20px', display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <div style={{ fontSize: '15px', fontWeight: 700, color: '#ffffff', marginBottom: '8px' }}>
            NSE Watchlist ({filteredStocks.length})
          </div>

          <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', overflowY: 'auto', maxHeight: '540px' }}>
            {filteredStocks.map((stock) => {
              const isSelected = selectedStock?.symbol === stock.symbol;
              const stockUp = stock.change_24h >= 0;
              return (
                <div
                  key={stock.symbol}
                  onClick={() => setSelectedSymbol(stock.symbol)}
                  style={{
                    padding: '12px 14px',
                    borderRadius: '12px',
                    backgroundColor: isSelected ? 'rgba(255, 255, 255, 0.08)' : 'rgba(255, 255, 255, 0.02)',
                    border: isSelected
                      ? '1px solid rgba(255, 255, 255, 0.16)'
                      : '1px solid rgba(255, 255, 255, 0.04)',
                    cursor: 'pointer',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    transition: 'all 0.15s ease',
                  }}
                >
                  <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
                    <div
                      style={{
                        width: '32px',
                        height: '32px',
                        borderRadius: '50%',
                        backgroundColor: stock.accent_color || '#8B5CF6',
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                        fontSize: '11px',
                        fontWeight: 700,
                        color: '#ffffff',
                      }}
                    >
                      {stock.symbol.slice(0, 2)}
                    </div>
                    <div>
                      <div style={{ fontWeight: 700, color: '#ffffff', fontSize: '13px' }}>
                        {stock.symbol}
                      </div>
                      <div style={{ fontSize: '11px', color: '#7e7a9c' }}>{stock.name}</div>
                    </div>
                  </div>

                  <div style={{ textAlign: 'right' }}>
                    <div className="mono" style={{ fontSize: '13px', fontWeight: 700, color: '#ffffff' }}>
                      {formatPaiseToINR(stock.current_price)}
                    </div>
                    <span className={`pill-badge ${stockUp ? 'pill-green' : 'pill-red'}`} style={{ fontSize: '10px' }}>
                      {formatPercent(stock.change_24h)}
                    </span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>
    </div>
  );
}
