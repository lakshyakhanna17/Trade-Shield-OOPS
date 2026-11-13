/**
 * TradeShield Stock Highlight Card
 * Pixel-accurate implementation of the cards from the reference screenshot:
 * - Rounded frosted card
 * - Stock badge + Ticker + Subtitle + Options button
 * - Vertical colored accent bar beside price
 * - Large price display in white
 * - Percentage change pill (+/-)
 * - Flowing Recharts AreaChart with subtle gradient fill
 */

import { MoreVertical, TrendingUp, TrendingDown } from 'lucide-react';
import { AreaChart, Area, ResponsiveContainer, YAxis, Tooltip } from 'recharts';
import { formatPaiseToINR, formatPercent } from '../../utils/formatters';

export default function StockCard({ stock, onTradeClick }) {
  const isUp = stock.change_24h >= 0;
  const accentColor = stock.accent_color || '#8B5CF6';

  return (
    <div
      className="glass-card"
      style={{
        padding: '20px 20px 14px',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        minHeight: '230px',
        cursor: 'pointer',
        minWidth: 0,
      }}
      onClick={() => onTradeClick && onTradeClick(stock)}
    >
      {/* Top Header */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
          {/* Stock Initials Badge */}
          <div
            style={{
              width: '40px',
              height: '40px',
              borderRadius: '50%',
              background: `linear-gradient(135deg, ${accentColor} 0%, rgba(20, 18, 30, 0.9) 100%)`,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              fontWeight: 800,
              fontSize: '13px',
              color: '#ffffff',
              boxShadow: `0 4px 14px ${accentColor}33`,
              border: '1px solid rgba(255, 255, 255, 0.15)',
            }}
          >
            {stock.symbol.slice(0, 2)}
          </div>

          <div>
            <div style={{ fontSize: '11px', color: '#8b87a8', fontWeight: 600, letterSpacing: '0.04em' }}>
              {stock.symbol}/INR
            </div>
            <div
              style={{
                fontSize: '15px',
                fontWeight: 700,
                color: '#ffffff',
                letterSpacing: '-0.02em',
                lineHeight: 1.2,
              }}
            >
              {stock.name}
            </div>
          </div>
        </div>

        {/* Options Button */}
        <button
          onClick={(e) => {
            e.stopPropagation();
            if (onTradeClick) onTradeClick(stock);
          }}
          style={{
            width: '28px',
            height: '28px',
            borderRadius: '50%',
            backgroundColor: 'rgba(255, 255, 255, 0.04)',
            border: '1px solid rgba(255, 255, 255, 0.08)',
            color: '#8b87a8',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            cursor: 'pointer',
          }}
        >
          <MoreVertical size={14} />
        </button>
      </div>

      {/* Price & Change Row with Left Accent Bar */}
      <div style={{ margin: '16px 0 6px', position: 'relative', paddingLeft: '14px' }}>
        {/* Left vertical colored accent bar (from reference screenshot) */}
        <div
          style={{
            position: 'absolute',
            left: 0,
            top: '4px',
            bottom: '4px',
            width: '3.5px',
            backgroundColor: accentColor,
            borderRadius: '999px',
            boxShadow: `0 0 10px ${accentColor}88`,
          }}
        />

        <div style={{ fontSize: '11px', color: '#7a7698', fontWeight: 500 }}>Price</div>

        <div style={{ display: 'flex', alignItems: 'baseline', gap: '10px', marginTop: '2px' }}>
          <div
            style={{
              fontSize: '24px',
              fontWeight: 800,
              color: '#ffffff',
              letterSpacing: '-0.03em',
            }}
          >
            {formatPaiseToINR(stock.current_price)}
          </div>

          {/* Change Pill */}
          <div
            className={`pill-badge ${isUp ? 'pill-green' : 'pill-red'}`}
            style={{ fontSize: '11px', padding: '3px 8px' }}
          >
            {isUp ? <TrendingUp size={12} /> : <TrendingDown size={12} />}
            <span>{formatPercent(stock.change_24h)}</span>
          </div>
        </div>
      </div>

      {/* Sparkline Chart */}
      <div style={{ width: '100%', height: '70px', marginTop: 'auto' }}>
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={stock.history || []} margin={{ top: 5, right: 0, left: 0, bottom: 0 }}>
            <defs>
              <linearGradient id={`grad-${stock.symbol}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor={accentColor} stopOpacity={0.35} />
                <stop offset="100%" stopColor={accentColor} stopOpacity={0.0} />
              </linearGradient>
            </defs>
            <YAxis hide domain={[(min) => Math.floor(min * 0.995), (max) => Math.ceil(max * 1.005)]} />
            <Tooltip
              content={({ active, payload }) => {
                if (active && payload && payload.length) {
                  return (
                    <div
                      style={{
                        backgroundColor: '#161423',
                        border: '1px solid rgba(255, 255, 255, 0.1)',
                        padding: '4px 8px',
                        borderRadius: '6px',
                        fontSize: '11px',
                        color: '#ffffff',
                        boxShadow: '0 4px 12px rgba(0,0,0,0.4)',
                      }}
                    >
                      <div style={{ color: '#8b87a8', fontSize: '10px' }}>{payload[0].payload?.time || 'Intraday'}</div>
                      <div style={{ fontWeight: 700, color: accentColor }}>₹{Number(payload[0].value).toFixed(2)}</div>
                    </div>
                  );
                }
                return null;
              }}
            />
            <Area
              type="monotone"
              dataKey="price"
              stroke={accentColor}
              strokeWidth={2}
              fillOpacity={1}
              fill={`url(#grad-${stock.symbol})`}
              dot={false}
              activeDot={{ r: 4, fill: '#ffffff', stroke: accentColor, strokeWidth: 2 }}
            />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
