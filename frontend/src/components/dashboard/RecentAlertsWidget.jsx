/**
 * TradeShield Recent Alerts Feed Widget
 * Highlights active fraud & anomaly alerts detected by the Shield Engine.
 */

import { Link } from 'react-router-dom';
import { ShieldAlert, ArrowRight, ShieldCheck } from 'lucide-react';
import { formatRelativeTime } from '../../utils/formatters';

export default function RecentAlertsWidget({ alerts }) {
  const safeAlerts = Array.isArray(alerts) ? alerts : (alerts?.alerts || []);
  const openAlerts = safeAlerts.filter((a) => a.review_status === 'OPEN').slice(0, 4);

  return (
    <div className="glass-card" style={{ padding: '22px 22px 18px', height: '100%', display: 'flex', flexDirection: 'column' }}>
      {/* Header */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '16px' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
          <div
            style={{
              width: '28px',
              height: '28px',
              borderRadius: '8px',
              backgroundColor: 'rgba(239, 68, 68, 0.15)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#f87171',
            }}
          >
            <ShieldAlert size={16} />
          </div>
          <div>
            <h4 style={{ fontSize: '15px', fontWeight: 700, color: '#ffffff' }}>Shield Anomaly Feed</h4>
            <div style={{ fontSize: '11px', color: '#7e7a9c' }}>Live Fraud Detection Activity</div>
          </div>
        </div>

        <Link
          to="/shield"
          style={{
            fontSize: '11px',
            fontWeight: 600,
            color: '#a78bfa',
            textDecoration: 'none',
            display: 'flex',
            alignItems: 'center',
            gap: '4px',
          }}
        >
          View All <ArrowRight size={12} />
        </Link>
      </div>

      {/* Feed list */}
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: '10px' }}>
        {openAlerts.length === 0 ? (
          <div
            style={{
              padding: '24px 16px',
              textAlign: 'center',
              color: '#7e7a9c',
              fontSize: '12px',
              backgroundColor: 'rgba(255, 255, 255, 0.02)',
              borderRadius: '12px',
            }}
          >
            <ShieldCheck size={28} color="#10b981" style={{ margin: '0 auto 8px', display: 'block' }} />
            No open fraud alerts. Market activity within normal bounds.
          </div>
        ) : (
          openAlerts.map((alert) => {
            const isHigh = alert.severity === 'HIGH' || alert.severity === 'CRITICAL';
            return (
              <div
                key={alert.alert_id}
                style={{
                  padding: '10px 12px',
                  borderRadius: '12px',
                  backgroundColor: 'rgba(255, 255, 255, 0.025)',
                  border: '1px solid rgba(255, 255, 255, 0.05)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'space-between',
                  gap: '12px',
                  transition: 'background 0.15s ease',
                }}
              >
                <div style={{ minWidth: 0, flex: 1 }}>
                  <div style={{ display: 'flex', alignItems: 'center', gap: '6px', marginBottom: '3px' }}>
                    <span
                      className={`pill-badge ${isHigh ? 'pill-red' : 'pill-amber'}`}
                      style={{ fontSize: '10px', padding: '2px 6px' }}
                    >
                      {alert.rule_name}
                    </span>
                    <span style={{ fontSize: '11px', fontWeight: 600, color: '#e2e0f0' }}>
                      {alert.symbol}
                    </span>
                  </div>
                  <div
                    style={{
                      fontSize: '11px',
                      color: '#8b87a8',
                      overflow: 'hidden',
                      textOverflow: 'ellipsis',
                      whiteSpace: 'nowrap',
                    }}
                    title={alert.description}
                  >
                    {alert.trader_name || `Trader #${alert.trader_id}`}: {alert.description}
                  </div>
                </div>

                <div style={{ textAlign: 'right', flexShrink: 0 }}>
                  <div
                    style={{
                      fontSize: '12px',
                      fontWeight: 700,
                      color: isHigh ? '#f87171' : '#fbbf24',
                    }}
                  >
                    {alert.risk_points} pts
                  </div>
                  <div style={{ fontSize: '10px', color: '#686484' }}>
                    {formatRelativeTime(alert.created_at)}
                  </div>
                </div>
              </div>
            );
          })
        )}
      </div>
    </div>
  );
}
