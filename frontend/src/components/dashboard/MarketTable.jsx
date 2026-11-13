/**
 * TradeShield Market Overview Table
 * Faithful recreation of the table in the lower half of the reference screenshot.
 */

import { useState } from 'react';
import { Clock, ArrowUpDown, Sliders, Shield } from 'lucide-react';
import { formatPaiseToINR, formatPercent, formatNumberIN } from '../../utils/formatters';

export default function MarketTable({ stocks, onSelectStock }) {
  const [filter, setFilter] = useState('ALL');
  const [sortField, setSortField] = useState('volume');
  const [sortAsc, setSortAsc] = useState(false);

  const handleSort = (field) => {
    if (sortField === field) {
      setSortAsc(!sortAsc);
    } else {
      setSortField(field);
      setSortAsc(false);
    }
  };

  const filteredStocks = stocks.filter((stock) => {
    if (filter === 'GAINERS') return stock.change_24h > 0;
    if (filter === 'LOSERS') return stock.change_24h < 0;
    if (filter === 'RISK') return stock.shield_risk !== 'LOW';
    return true;
  });

  const sortedStocks = [...filteredStocks].sort((a, b) => {
    let aVal = a[sortField];
    let bVal = b[sortField];
    if (typeof aVal === 'string') {
      return sortAsc ? aVal.localeCompare(bVal) : bVal.localeCompare(aVal);
    }
    return sortAsc ? aVal - bVal : bVal - aVal;
  });

  return (
    <div className="glass-card" style={{ padding: '20px 20px 14px', minWidth: 0 }}>
      {/* Table Section Header */}
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          marginBottom: '16px',
          flexWrap: 'wrap',
          gap: '12px',
        }}
      >
        <div>
          <div
            style={{
              display: 'flex',
              alignItems: 'center',
              gap: '6px',
              fontSize: '11px',
              fontWeight: 600,
              color: '#8b87a8',
              textTransform: 'uppercase',
              letterSpacing: '0.05em',
            }}
          >
            <Clock size={12} />
            Live Updates
          </div>
          <h3
            style={{
              fontSize: '20px',
              fontWeight: 700,
              color: '#ffffff',
              letterSpacing: '-0.02em',
              marginTop: '2px',
            }}
          >
            Market Overview
          </h3>
        </div>

        {/* Filter Tabs */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '6px', flexWrap: 'wrap' }}>
          {['ALL', 'GAINERS', 'LOSERS', 'RISK'].map((tab) => (
            <button
              key={tab}
              onClick={() => setFilter(tab)}
              className="btn btn-sm"
              style={{
                backgroundColor: filter === tab ? 'rgba(255, 255, 255, 0.08)' : 'transparent',
                color: filter === tab ? '#ffffff' : '#7e7a9c',
                border: filter === tab ? '1px solid rgba(255, 255, 255, 0.12)' : '1px solid transparent',
                padding: '6px 12px',
                fontSize: '12px',
              }}
            >
              {tab === 'ALL' ? 'All Stocks' : tab}
            </button>
          ))}
        </div>
      </div>

      {/* Table */}
      <div className="table-responsive">
        <table className="trade-table">
          <thead>
            <tr>
              <th style={{ width: '48px' }}>No</th>
              <th style={{ cursor: 'pointer' }} onClick={() => handleSort('name')}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
                  Stock Name <ArrowUpDown size={11} />
                </div>
              </th>
              <th style={{ cursor: 'pointer' }} onClick={() => handleSort('current_price')}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
                  Price (₹) <ArrowUpDown size={11} />
                </div>
              </th>
              <th style={{ cursor: 'pointer' }} onClick={() => handleSort('change_24h')}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
                  24h % <ArrowUpDown size={11} />
                </div>
              </th>
              <th style={{ cursor: 'pointer' }} onClick={() => handleSort('change_7d')}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
                  7D % <ArrowUpDown size={11} />
                </div>
              </th>
              <th style={{ cursor: 'pointer' }} onClick={() => handleSort('volume')}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
                  Volume <ArrowUpDown size={11} />
                </div>
              </th>
              <th>Shield Risk</th>
              <th style={{ textAlign: 'right' }}>Action</th>
            </tr>
          </thead>
          <tbody>
            {sortedStocks.map((stock, index) => {
              const is24hUp = stock.change_24h >= 0;
              const is7dUp = stock.change_7d >= 0;
              const accentColor = stock.accent_color || '#8B5CF6';

              return (
                <tr key={stock.symbol}>
                  {/* No */}
                  <td style={{ color: '#686484', fontWeight: 600, fontSize: '12px' }}>
                    #{index + 1}
                  </td>

                  {/* Stock Symbol & Name */}
                  <td>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
                      <div
                        style={{
                          width: '32px',
                          height: '32px',
                          borderRadius: '50%',
                          background: `linear-gradient(135deg, ${accentColor} 0%, rgba(15, 14, 24, 0.9) 100%)`,
                          display: 'flex',
                          alignItems: 'center',
                          justifyContent: 'center',
                          fontWeight: 700,
                          fontSize: '11px',
                          color: '#ffffff',
                          border: '1px solid rgba(255, 255, 255, 0.1)',
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
                  </td>

                  {/* Price */}
                  <td className="mono" style={{ fontWeight: 600, color: '#ffffff' }}>
                    {formatPaiseToINR(stock.current_price)}
                  </td>

                  {/* 24h % */}
                  <td>
                    <span className={`pill-badge ${is24hUp ? 'pill-green' : 'pill-red'}`}>
                      {formatPercent(stock.change_24h)}
                    </span>
                  </td>

                  {/* 7D % */}
                  <td>
                    <span className={`pill-badge ${is7dUp ? 'pill-green' : 'pill-red'}`}>
                      {formatPercent(stock.change_7d)}
                    </span>
                  </td>

                  {/* Volume */}
                  <td className="mono" style={{ color: '#b2aed4' }}>
                    {formatNumberIN(stock.volume)}
                  </td>

                  {/* Shield Risk */}
                  <td>
                    <span
                      className={`pill-badge ${
                        stock.shield_risk === 'HIGH'
                          ? 'pill-red'
                          : stock.shield_risk === 'MEDIUM'
                          ? 'pill-amber'
                          : 'pill-green'
                      }`}
                      style={{ fontSize: '10px' }}
                    >
                      <Shield size={10} />
                      {stock.shield_risk || 'LOW'}
                    </span>
                  </td>

                  {/* Action */}
                  <td style={{ textAlign: 'right' }}>
                    <button
                      onClick={() => onSelectStock && onSelectStock(stock)}
                      className="btn btn-ghost btn-sm"
                      style={{ fontSize: '11px', gap: '4px' }}
                    >
                      <Sliders size={12} />
                      Trade
                    </button>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    </div>
  );
}
