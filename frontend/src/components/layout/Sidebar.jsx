/**
 * TradeShield Navigation Sidebar
 * Faithful recreation of the reference screenshot's dark fintech aesthetic:
 * - Brand emblem & name
 * - User greeting snippet
 * - Categorized navigation with active pill highlight and right indicator
 * - Alert and simulation badges
 */

import { NavLink } from 'react-router-dom';
import {
  LayoutDashboard,
  PieChart,
  TrendingUp,
  ArrowLeftRight,
  ShieldAlert,
  Bot,
  Sliders,
  ChevronLeft,
  Shield,
  Layers,
  X,
} from 'lucide-react';
import { useTradeShield } from '../../context/useTradeShield';

export default function Sidebar({ collapsed, onToggleCollapse, mobileOpen, onCloseMobile }) {
  const { alerts, simulation, currentTrader } = useTradeShield();

  // Active open alerts count with defensive array guard
  const safeAlerts = Array.isArray(alerts) ? alerts : (alerts?.alerts || []);
  const openAlertsCount = safeAlerts.filter((a) => a.review_status === 'OPEN').length;
  const isSimRunning = Boolean(simulation?.is_running);

  const navSections = [
    {
      title: 'Overview',
      items: [
        { label: 'Dashboard', path: '/', icon: LayoutDashboard },
        { label: 'Market Overview', path: '/market', icon: TrendingUp },
      ],
    },
    {
      title: 'Account',
      items: [
        { label: 'Portfolio', path: '/portfolio', icon: PieChart },
      ],
    },
    {
      title: 'Activity',
      items: [
        { label: 'Trade', path: '/trading', icon: Sliders },
        { label: 'Transactions', path: '/transactions', icon: ArrowLeftRight },
      ],
    },
    {
      title: 'Shield Intelligence',
      items: [
        {
          label: 'Shield Monitor',
          path: '/shield',
          icon: ShieldAlert,
          badge: openAlertsCount > 0 ? openAlertsCount : null,
          badgeColor: 'pill-red',
        },
        {
          label: 'Bot Simulation',
          path: '/simulation',
          icon: Bot,
          badge: isSimRunning ? 'LIVE' : null,
          badgeColor: 'pill-purple',
          pulse: isSimRunning,
        },
      ],
    },
  ];

  return (
    <aside
      className={`sidebar-root ${mobileOpen ? 'mobile-open' : ''}`}
      style={{
        width: collapsed ? '80px' : '270px',
        minWidth: collapsed ? '80px' : '270px',
        backgroundColor: '#0c0a13',
        borderRight: '1px solid rgba(255, 255, 255, 0.06)',
        display: 'flex',
        flexDirection: 'column',
        height: '100vh',
        position: 'sticky',
        top: 0,
        zIndex: 50,
        transition: 'all 0.3s cubic-bezier(0.16, 1, 0.3, 1)',
        overflowY: 'auto',
        overflowX: 'hidden',
      }}
    >
      {/* Brand Header */}
      <div
        style={{
          padding: '24px 20px 20px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          borderBottom: '1px solid rgba(255, 255, 255, 0.04)',
        }}
      >
        <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
          {/* Logo Icon */}
          <div
            style={{
              width: '38px',
              height: '38px',
              borderRadius: '12px',
              background: 'linear-gradient(135deg, #7c3aed 0%, #4f46e5 100%)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              boxShadow: '0 4px 14px rgba(124, 58, 237, 0.4)',
              color: '#ffffff',
              flexShrink: 0,
            }}
          >
            <Shield size={20} />
          </div>

          <div>
            <div
              style={{
                fontSize: '17px',
                fontWeight: 800,
                letterSpacing: '-0.03em',
                color: '#ffffff',
                lineHeight: 1.1,
              }}
            >
              TradeShield
            </div>
            <div
              style={{
                fontSize: '11px',
                fontWeight: 500,
                color: '#8b87a8',
                letterSpacing: '-0.01em',
              }}
            >
              AI-Powered Risk Engine
            </div>
          </div>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
          {/* Mobile Close Button */}
          <button
            onClick={onCloseMobile}
            className="mobile-close-btn"
            title="Close sidebar"
            aria-label="Close sidebar"
          >
            <X size={16} />
          </button>

          {/* Desktop Collapse Button */}
          <button
            onClick={onToggleCollapse}
            className="sidebar-collapse-btn"
            title={collapsed ? 'Expand sidebar' : 'Collapse sidebar'}
            style={{
              width: '28px',
              height: '28px',
              borderRadius: '8px',
              backgroundColor: 'rgba(255, 255, 255, 0.04)',
              border: '1px solid rgba(255, 255, 255, 0.08)',
              color: '#a78bfa',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              cursor: 'pointer',
              transition: 'all 0.2s ease',
            }}
          >
            <ChevronLeft
              size={16}
              style={{
                transform: collapsed ? 'rotate(180deg)' : 'none',
                transition: 'transform 0.3s ease',
              }}
            />
          </button>
        </div>
      </div>

      {/* Greeting Header (As shown in screenshot) */}
      {!collapsed && (
        <div
          style={{
            padding: '22px 20px 14px',
          }}
        >
          <h2
            style={{
              fontSize: '22px',
              fontWeight: 700,
              color: '#ffffff',
              letterSpacing: '-0.03em',
              lineHeight: 1.2,
            }}
          >
            Welcome<br />Back, {currentTrader.name.split(' ')[0]}
          </h2>
          <p
            style={{
              fontSize: '12px',
              color: '#7e7a9c',
              marginTop: '4px',
            }}
          >
            Last login: 11 Nov 2026
          </p>
        </div>
      )}

      {/* Navigation Sections */}
      <nav
        style={{
          flex: 1,
          padding: '12px 14px',
          display: 'flex',
          flexDirection: 'column',
          gap: '18px',
        }}
      >
        {navSections.map((section) => (
          <div key={section.title}>
            {!collapsed && (
              <div
                style={{
                  fontSize: '11px',
                  fontWeight: 600,
                  textTransform: 'uppercase',
                  letterSpacing: '0.06em',
                  color: '#5d587c',
                  padding: '4px 10px 8px',
                }}
              >
                {section.title}
              </div>
            )}

            <div style={{ display: 'flex', flexDirection: 'column', gap: '3px' }}>
              {section.items.map((item) => {
                const Icon = item.icon;
                return (
                  <NavLink
                    key={item.path}
                    to={item.path}
                    onClick={() => onCloseMobile && onCloseMobile()}
                    title={collapsed ? item.label : undefined}
                    style={({ isActive }) => ({
                      display: 'flex',
                      alignItems: 'center',
                      gap: '12px',
                      padding: collapsed ? '11px 0' : '10px 14px',
                      justifyContent: collapsed ? 'center' : 'flex-start',
                      borderRadius: '12px',
                      color: isActive ? '#ffffff' : '#8e8aa8',
                      backgroundColor: isActive ? 'rgba(255, 255, 255, 0.05)' : 'transparent',
                      border: isActive
                        ? '1px solid rgba(255, 255, 255, 0.1)'
                        : '1px solid transparent',
                      boxShadow: isActive
                        ? '0 4px 16px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255, 255, 255, 0.12)'
                        : 'none',
                      textDecoration: 'none',
                      fontSize: '13px',
                      fontWeight: isActive ? 600 : 500,
                      position: 'relative',
                      transition: 'all 0.18s ease',
                    })}
                  >
                    {({ isActive }) => (
                      <>
                        <Icon
                          size={18}
                          color={isActive ? '#a78bfa' : '#777394'}
                          strokeWidth={isActive ? 2.2 : 1.8}
                        />

                        {!collapsed && (
                          <span style={{ flex: 1, whiteSpace: 'nowrap' }}>{item.label}</span>
                        )}

                        {/* Badges */}
                        {!collapsed && item.badge && (
                          <span
                            className={`pill-badge ${item.badgeColor}`}
                            style={{
                              padding: '2px 7px',
                              fontSize: '10px',
                              fontWeight: 700,
                            }}
                          >
                            {item.pulse && (
                              <span
                                className="pulse-indicator"
                                style={{ backgroundColor: '#a78bfa', width: '6px', height: '6px' }}
                              />
                            )}
                            {item.badge}
                          </span>
                        )}

                        {/* Active vertical right indicator pill */}
                        {isActive && !collapsed && (
                          <div
                            style={{
                              position: 'absolute',
                              right: '8px',
                              width: '3px',
                              height: '16px',
                              backgroundColor: '#ffffff',
                              borderRadius: '999px',
                              boxShadow: '0 0 8px rgba(255, 255, 255, 0.8)',
                            }}
                          />
                        )}
                      </>
                    )}
                  </NavLink>
                );
              })}
            </div>
          </div>
        ))}
      </nav>

      {/* Footer System Status */}
      {!collapsed && (
        <div
          style={{
            padding: '16px 18px',
            margin: '0 12px 14px',
            backgroundColor: 'rgba(255, 255, 255, 0.02)',
            border: '1px solid rgba(255, 255, 255, 0.05)',
            borderRadius: '12px',
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
          }}
        >
          <div
            style={{
              width: '8px',
              height: '8px',
              borderRadius: '50%',
              backgroundColor: '#10b981',
              boxShadow: '0 0 10px rgba(16, 185, 129, 0.7)',
            }}
          />
          <div style={{ flex: 1 }}>
            <div style={{ fontSize: '11px', fontWeight: 600, color: '#c4c0e0' }}>
              Shield Engine
            </div>
            <div style={{ fontSize: '10px', color: '#686484' }}>v1.0.0 • SQLite Active</div>
          </div>
          <Layers size={14} color="#8b87a8" />
        </div>
      )}
    </aside>
  );
}
