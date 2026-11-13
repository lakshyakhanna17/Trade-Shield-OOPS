/**
 * TradeShield Header & Ticker Bar (Stage 8)
 * Features:
 * - Breadcrumb path matching screenshot (Home icon + Section / Page)
 * - Horizontal compact live stock ticker strip
 * - Global search bar
 * - Explicit backend connection status indicator:
 *   [CONNECTED (C++ Core Live) | STANDALONE MODE | BACKEND DISCONNECTED]
 * - Refresh & mode toggle controls
 * - User avatar
 */

import { useLocation } from 'react-router-dom';
import { Home, Search, RefreshCw, Menu } from 'lucide-react';
import { useTradeShield } from '../../context/useTradeShield';
import { formatPaiseToINR, formatPercent } from '../../utils/formatters';

export default function Topbar({ onOpenMobileMenu }) {
  const location = useLocation();
  const {
    stocks,
    searchQuery,
    setSearchQuery,
    connectionStatus,
    toggleStandaloneMode,
    refreshData,
    isRefreshing,
    currentTrader,
  } = useTradeShield();

  // Map route to breadcrumb label
  const getBreadcrumb = () => {
    switch (location.pathname) {
      case '/':
        return { section: 'Overview', page: 'Dashboard' };
      case '/market':
        return { section: 'Overview', page: 'Market Overview' };
      case '/portfolio':
        return { section: 'Account', page: 'Portfolio & Holdings' };
      case '/trading':
        return { section: 'Activity', page: 'Trade Terminal' };
      case '/transactions':
        return { section: 'Activity', page: 'Transactions & Audit' };
      case '/shield':
        return { section: 'Shield Intelligence', page: 'Fraud & Risk Monitor' };
      case '/simulation':
        return { section: 'Shield Intelligence', page: 'Bot Simulation Center' };
      default:
        return { section: 'Overview', page: 'Dashboard' };
    }
  };

  const breadcrumb = getBreadcrumb();

  // Connection badge config
  const getConnectionBadge = () => {
    switch (connectionStatus) {
      case 'CONNECTED':
        return {
          pillClass: 'pill-green',
          dotColor: '#10b981',
          label: 'C++ Core Live',
          title: 'Connected to C++17 Crow Backend on :8080 (SQLite active)',
        };
      case 'STANDALONE':
        return {
          pillClass: 'pill-purple',
          dotColor: '#a78bfa',
          label: 'Standalone Mode',
          title: 'Operating with local mock dataset. Click to reconnect to backend.',
        };
      case 'DISCONNECTED':
        return {
          pillClass: 'pill-red',
          dotColor: '#ef4444',
          label: 'Backend Offline',
          title: 'C++ Backend unreachable at :8080. Click to retry.',
        };
      default:
        return {
          pillClass: 'pill-amber',
          dotColor: '#f59e0b',
          label: 'Connecting...',
          title: 'Probing C++ Backend on :8080',
        };
    }
  };

  const badge = getConnectionBadge();

  return (
    <header
      style={{
        height: 'var(--topbar-height)',
        backgroundColor: 'rgba(12, 10, 19, 0.75)',
        backdropFilter: 'blur(20px)',
        WebkitBackdropFilter: 'blur(20px)',
        borderBottom: '1px solid rgba(255, 255, 255, 0.06)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: '0 18px',
        position: 'sticky',
        top: 0,
        zIndex: 40,
        gap: '12px',
      }}
    >
      {/* Left Area: Hamburger (Mobile) + Breadcrumb */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '10px', minWidth: 0 }}>
        {/* Hamburger Menu Toggle on Mobile */}
        <button
          onClick={onOpenMobileMenu}
          className="mobile-menu-btn"
          title="Open navigation menu"
          aria-label="Open navigation menu"
        >
          <Menu size={18} />
        </button>

        {/* Breadcrumb */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px', minWidth: 0 }}>
          <div
            style={{
              width: '26px',
              height: '26px',
              borderRadius: '6px',
              backgroundColor: 'rgba(255, 255, 255, 0.04)',
              border: '1px solid rgba(255, 255, 255, 0.08)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#d1d5db',
              flexShrink: 0,
            }}
          >
            <Home size={13} />
          </div>
          <div style={{ fontSize: '13px', display: 'flex', alignItems: 'center', gap: '6px', minWidth: 0 }}>
            <span className="mobile-hide" style={{ color: '#7e7a9c', whiteSpace: 'nowrap' }}>{breadcrumb.section}</span>
            <span className="mobile-hide" style={{ color: '#4d4968' }}>/</span>
            <span
              style={{
                color: '#ffffff',
                fontWeight: 600,
                whiteSpace: 'nowrap',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
              }}
            >
              {breadcrumb.page}
            </span>
          </div>
        </div>
      </div>

      {/* Compact Live Ticker Strip (Desktop & Tablet only) */}
      <div
        className="mobile-hide"
        style={{
          flex: 1,
          display: 'flex',
          alignItems: 'center',
          gap: '12px',
          overflowX: 'auto',
          padding: '4px 8px',
          scrollbarWidth: 'none',
          minWidth: 0,
        }}
      >
        {stocks.slice(0, 6).map((stock) => {
          const isUp = (stock.change_24h || 0) >= 0;
          return (
            <div
              key={stock.symbol}
              style={{
                display: 'flex',
                alignItems: 'center',
                gap: '8px',
                fontSize: '11px',
                whiteSpace: 'nowrap',
                backgroundColor: 'rgba(255, 255, 255, 0.02)',
                padding: '4px 10px',
                borderRadius: '8px',
                border: '1px solid rgba(255, 255, 255, 0.04)',
                flexShrink: 0,
              }}
            >
              <span style={{ fontWeight: 700, color: '#c4c0e0' }}>{stock.symbol}</span>
              <span className="mono" style={{ color: '#ffffff' }}>
                {formatPaiseToINR(stock.current_price, false)}
              </span>
              <span
                style={{
                  color: isUp ? '#34d399' : '#f87171',
                  fontWeight: 600,
                  fontSize: '10px',
                }}
              >
                {formatPercent(stock.change_24h || 0)}
              </span>
            </div>
          );
        })}
      </div>

      {/* Right Controls */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '8px', flexShrink: 0 }}>
        {/* Search Bar (Hidden on small mobile screens to prevent header overflow) */}
        <div
          className="mobile-small-hide"
          style={{
            position: 'relative',
            display: 'flex',
            alignItems: 'center',
            width: '160px',
          }}
        >
          <Search
            size={13}
            color="#7e7a9c"
            style={{ position: 'absolute', left: '10px', pointerEvents: 'none' }}
          />
          <input
            type="text"
            placeholder="Search..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            style={{
              width: '100%',
              backgroundColor: 'rgba(20, 18, 30, 0.9)',
              border: '1px solid rgba(255, 255, 255, 0.08)',
              borderRadius: '999px',
              padding: '6px 12px 6px 28px',
              fontSize: '12px',
              color: '#ffffff',
              outline: 'none',
            }}
          />
        </div>

        {/* Backend Status Badge / Mode Switcher */}
        <button
          onClick={toggleStandaloneMode}
          className={`pill-badge ${badge.pillClass}`}
          title={badge.title}
          style={{
            cursor: 'pointer',
            border: 'none',
            outline: 'none',
            padding: '5px 10px',
            fontSize: '11px',
            whiteSpace: 'nowrap',
          }}
        >
          <span
            className="pulse-indicator"
            style={{
              backgroundColor: badge.dotColor,
              width: '6px',
              height: '6px',
            }}
          />
          <span className="mobile-small-hide">{badge.label}</span>
          <span className="desktop-only">{badge.label}</span>
        </button>

        {/* Refresh Button */}
        <button
          onClick={() => !isRefreshing && refreshData()}
          disabled={isRefreshing}
          title="Refresh market, orders, and Shield telemetry from C++ server"
          style={{
            width: '32px',
            height: '32px',
            borderRadius: '8px',
            backgroundColor: 'rgba(255, 255, 255, 0.04)',
            border: '1px solid rgba(255, 255, 255, 0.08)',
            color: '#d1d5db',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            cursor: isRefreshing ? 'default' : 'pointer',
            opacity: isRefreshing ? 0.75 : 1,
            transition: 'opacity 0.2s ease',
            flexShrink: 0,
          }}
        >
          <RefreshCw
            size={14}
            className={isRefreshing ? 'spin-icon' : ''}
          />
        </button>

        {/* User Profile Chip */}
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: '6px',
            padding: '3px 8px 3px 3px',
            backgroundColor: 'rgba(255, 255, 255, 0.04)',
            borderRadius: '999px',
            border: '1px solid rgba(255, 255, 255, 0.08)',
            flexShrink: 0,
          }}
        >
          <div
            style={{
              width: '26px',
              height: '26px',
              borderRadius: '50%',
              background: 'linear-gradient(135deg, #4b5563 0%, #1f2937 100%)',
              border: '1px solid rgba(255, 255, 255, 0.2)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              fontWeight: 700,
              fontSize: '11px',
              color: '#ffffff',
            }}
          >
            {currentTrader.name.charAt(0)}
          </div>
          <span className="mobile-small-hide" style={{ fontSize: '12px', fontWeight: 600, color: '#e2e0f0' }}>
            {currentTrader.name.split(' ')[0]}
          </span>
        </div>
      </div>
    </header>
  );
}
