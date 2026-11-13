/**
 * TradeShield Main Application Layout
 * Responsive shell wrapping Sidebar, Topbar, Toasts, and Page Content.
 */

import { useState } from 'react';
import { Outlet } from 'react-router-dom';
import Sidebar from './Sidebar';
import Topbar from './Topbar';
import ErrorBoundary from '../common/ErrorBoundary';
import { useTradeShield } from '../../context/useTradeShield';
import { CheckCircle2, AlertTriangle, Info } from 'lucide-react';

export default function MainLayout() {
  const [collapsed, setCollapsed] = useState(false);
  const [mobileOpen, setMobileOpen] = useState(false);
  const { toast, connectionStatus, refreshData } = useTradeShield();

  const getToastIcon = (type) => {
    switch (type) {
      case 'success':
        return <CheckCircle2 size={16} color="#34d399" />;
      case 'error':
        return <AlertTriangle size={16} color="#f87171" />;
      default:
        return <Info size={16} color="#d1d5db" />;
    }
  };

  return (
    <div style={{ display: 'flex', minHeight: '100vh', position: 'relative', overflowX: 'hidden' }}>
      {/* Mobile Backdrop */}
      <div
        className={`mobile-sidebar-backdrop ${mobileOpen ? 'active' : ''}`}
        onClick={() => setMobileOpen(false)}
        aria-hidden="true"
      />

      {/* Sidebar */}
      <Sidebar
        collapsed={collapsed}
        onToggleCollapse={() => setCollapsed(!collapsed)}
        mobileOpen={mobileOpen}
        onCloseMobile={() => setMobileOpen(false)}
      />

      {/* Main Content Area */}
      <div
        style={{
          flex: 1,
          display: 'flex',
          flexDirection: 'column',
          minWidth: 0,
          position: 'relative',
          zIndex: 1,
        }}
      >
        <Topbar onOpenMobileMenu={() => setMobileOpen(true)} />

        {/* Backend Disconnection Alert Banner */}
        {connectionStatus === 'DISCONNECTED' && (
          <div
            style={{
              backgroundColor: 'rgba(239, 68, 68, 0.12)',
              borderBottom: '1px solid rgba(239, 68, 68, 0.3)',
              padding: '10px 18px',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'space-between',
              flexWrap: 'wrap',
              gap: '12px',
              color: '#fca5a5',
              fontSize: '13px',
              fontWeight: 500,
            }}
          >
            <div style={{ display: 'flex', alignItems: 'center', gap: '10px', minWidth: 0, flex: 1 }}>
              <AlertTriangle size={16} color="#ef4444" style={{ flexShrink: 0 }} />
              <span style={{ wordBreak: 'break-word' }}>
                <strong>Backend Disconnected:</strong> Cannot reach C++ TradeShield server on port 8080. Live trading and real-time updates are paused.
              </span>
            </div>
            <button
              onClick={refreshData}
              className="btn btn-sm"
              style={{
                backgroundColor: 'rgba(239, 68, 68, 0.25)',
                color: '#ffffff',
                border: '1px solid rgba(239, 68, 68, 0.4)',
                padding: '6px 14px',
                fontSize: '12px',
                cursor: 'pointer',
                flexShrink: 0,
              }}
            >
              Retry Connection
            </button>
          </div>
        )}

        <main className="main-content-layout">
          <ErrorBoundary>
            <Outlet />
          </ErrorBoundary>
        </main>
      </div>

      {/* Global Toast Notification */}
      {toast && (
        <div
          style={{
            position: 'fixed',
            bottom: '20px',
            right: '20px',
            maxWidth: 'calc(100vw - 40px)',
            backgroundColor: '#1b1928',
            border: '1px solid rgba(255, 255, 255, 0.14)',
            borderRadius: '12px',
            padding: '12px 18px',
            display: 'flex',
            alignItems: 'center',
            gap: '12px',
            boxShadow: '0 12px 36px rgba(0, 0, 0, 0.6)',
            zIndex: 9999,
            animation: 'modalFadeIn 0.2s cubic-bezier(0.16, 1, 0.3, 1)',
          }}
        >
          {getToastIcon(toast.type)}
          <span style={{ fontSize: '13px', color: '#ffffff', fontWeight: 500 }}>
            {toast.message}
          </span>
        </div>
      )}
    </div>
  );
}
