/**
 * TradeShield Shield Monitor Page
 * Advanced Fraud & Anomaly Intelligence Center:
 * - Risk severity distribution
 * - Trader risk scores leaderboard
 * - Real-time detection rule insights (HIGH_VOLUME, REPEATED_CANCELLATION, HIGH_FREQUENCY)
 * - Incident triage with Alert Detail modal and Review/Clear workflows
 */

import { useState } from 'react';
import {
  ShieldAlert,
  Flame,
  CheckCircle2,
  Eye,
  Check,
  X,
  Layers,
  Activity,
  Bot,
} from 'lucide-react';
import { useTradeShield } from '../context/useTradeShield';
import { formatDateTime, formatRelativeTime } from '../utils/formatters';

export default function Shield() {
  const { alerts, alertStats, reviewAlert, riskProfiles } = useTradeShield();
  const [selectedAlert, setSelectedAlert] = useState(null);
  const [severityFilter, setSeverityFilter] = useState('ALL');
  const [statusFilter, setStatusFilter] = useState('ALL');

  const safeAlerts = Array.isArray(alerts) ? alerts : (alerts?.alerts || []);

  // Metrics (sourced directly from SQLite aggregate stats endpoint)
  const totalAlerts = alertStats?.total_alerts ?? safeAlerts.length;
  const openAlerts = alertStats?.open_alerts ?? safeAlerts.filter((a) => a.review_status === 'OPEN').length;
  const reviewedAlerts = alertStats?.reviewed_alerts ?? safeAlerts.filter((a) => a.review_status === 'REVIEWED').length;
  const highCriticalAlerts = alertStats?.high_severity_alerts ?? alertStats?.high_critical_alerts ?? safeAlerts.filter(
    (a) => (a.severity === 'HIGH' || a.severity === 'CRITICAL') && a.review_status === 'OPEN'
  ).length;

  const filteredAlerts = safeAlerts.filter((a) => {
    if (severityFilter !== 'ALL' && a.severity !== severityFilter) return false;
    if (statusFilter !== 'ALL' && a.review_status !== statusFilter) return false;
    return true;
  });

  // Detection rules catalogue
  const detectionRules = [
    {
      name: 'HIGH_VOLUME',
      title: 'High-Volume Anomaly',
      desc: 'Detects isolated order spikes ≥100 shares or cumulative sustained volumes that exceed rolling trader baselines.',
      severity: 'HIGH / MEDIUM',
      color: '#f59e0b',
    },
    {
      name: 'REPEATED_CANCELLATION',
      title: 'Repeated Cancellation / Spoofing',
      desc: 'Flags predatory order placement and rapid cancellation bursts (≥3 cancels and ≥50% cancellation ratio in 60s window).',
      severity: 'HIGH / MEDIUM',
      color: '#ef4444',
    },
    {
      name: 'HIGH_FREQUENCY',
      title: 'High-Frequency Burst',
      desc: 'Identifies algorithmic high-rate order spamming exceeding 5 orders within a 10-second sliding evaluation window.',
      severity: 'HIGH / MEDIUM',
      color: '#8b5cf6',
    },
  ];

  // Trader risk scores leaderboard sourced dynamically from C++ FraudEngine
  const displayRisks = riskProfiles && riskProfiles.length > 0
    ? riskProfiles.map((p) => ({
        id: p.trader_id,
        name: p.trader_name,
        type: p.trader_id === 1 ? 'HUMAN' : 'BOT',
        score: p.overall_risk_score,
        tier: p.risk_tier,
        activeRules: p.triggered_rules || [],
      }))
    : [];

  const handleReview = async (alertId) => {
    await reviewAlert(alertId, 'REVIEWED');
    if (selectedAlert?.alert_id === alertId) {
      setSelectedAlert((prev) => (prev ? { ...prev, review_status: 'REVIEWED' } : null));
    }
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Header */}
      <div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '6px', flexWrap: 'wrap' }}>
          <span className="pill-badge pill-red">SHIELD INTELLIGENCE ENGINE</span>
          <span style={{ fontSize: '12px', color: '#8b87a8' }}>Sliding-Window Behavioral Analysis</span>
        </div>
        <h1 className="page-title">
          Fraud & Risk Monitor
        </h1>
        <p className="page-subtitle">
          Real-time pattern surveillance across orders, executions, and algorithmic trader behaviour.
        </p>
      </div>

      {/* Top 4 Risk Summary Cards */}
      <div className="responsive-stat-grid-4">
        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
            <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
              Active Incidents
            </div>
            <ShieldAlert size={16} color="#f87171" />
          </div>
          <div className="mono" style={{ fontSize: '28px', fontWeight: 800, color: '#f87171', marginTop: '6px' }}>
            {openAlerts}
          </div>
          <div style={{ fontSize: '11px', color: '#8b87a8', marginTop: '4px' }}>
            Awaiting Compliance Review
          </div>
        </div>

        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
            <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
              High/Critical Severity
            </div>
            <Flame size={16} color="#f97316" />
          </div>
          <div className="mono" style={{ fontSize: '28px', fontWeight: 800, color: '#fbbf24', marginTop: '6px' }}>
            {highCriticalAlerts}
          </div>
          <div style={{ fontSize: '11px', color: '#8b87a8', marginTop: '4px' }}>
            Immediate Risk Points &gt; 50
          </div>
        </div>

        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
            <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
              Reviewed / Cleared
            </div>
            <CheckCircle2 size={16} color="#10b981" />
          </div>
          <div className="mono" style={{ fontSize: '28px', fontWeight: 800, color: '#34d399', marginTop: '6px' }}>
            {reviewedAlerts}
          </div>
          <div style={{ fontSize: '11px', color: '#8b87a8', marginTop: '4px' }}>
            Closed Risk Events
          </div>
        </div>

        <div className="glass-card" style={{ padding: '20px' }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
            <div style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>
              Total Alerts Logged
            </div>
            <Layers size={16} color="#a78bfa" />
          </div>
          <div className="mono" style={{ fontSize: '28px', fontWeight: 800, color: '#ffffff', marginTop: '6px' }}>
            {totalAlerts}
          </div>
          <div style={{ fontSize: '11px', color: '#8b87a8', marginTop: '4px' }}>
            Persisted in SQLite
          </div>
        </div>
      </div>

      {/* Middle Section: Detection Rules Architecture + Trader Risk Scores */}
      <div className="responsive-grid-2col">
        {/* Detection Rules Information Card */}
        <div className="glass-card" style={{ padding: '24px', minWidth: 0 }}>
          <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff', marginBottom: '6px' }}>
            Shield Pattern Detection Rules
          </h3>
          <p style={{ fontSize: '12px', color: '#8b87a8', marginBottom: '16px' }}>
            TradeShield analyses multi-event behavioural patterns over sliding time windows rather than judging single transactions in isolation.
          </p>

          <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
            {detectionRules.map((rule) => (
              <div
                key={rule.name}
                style={{
                  padding: '14px 16px',
                  borderRadius: '12px',
                  backgroundColor: 'rgba(255, 255, 255, 0.02)',
                  border: '1px solid rgba(255, 255, 255, 0.05)',
                  display: 'flex',
                  alignItems: 'flex-start',
                  justifyContent: 'space-between',
                  gap: '14px',
                }}
              >
                <div>
                  <div style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '4px' }}>
                    <span
                      style={{
                        width: '8px',
                        height: '8px',
                        borderRadius: '50%',
                        backgroundColor: rule.color,
                        boxShadow: `0 0 8px ${rule.color}`,
                      }}
                    />
                    <span style={{ fontWeight: 700, color: '#ffffff', fontSize: '14px' }}>
                      {rule.title}
                    </span>
                    <span className="mono" style={{ fontSize: '11px', color: '#7e7a9c' }}>
                      ({rule.name})
                    </span>
                  </div>
                  <p style={{ fontSize: '12px', color: '#a7a3c8', lineHeight: 1.4 }}>
                    {rule.desc}
                  </p>
                </div>
                <span className="pill-badge pill-neutral" style={{ fontSize: '10px', flexShrink: 0 }}>
                  {rule.severity}
                </span>
              </div>
            ))}
          </div>
        </div>

        {/* Trader Risk Leaderboard */}
        <div className="glass-card" style={{ padding: '24px', display: 'flex', flexDirection: 'column', minWidth: 0 }}>
          <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff', marginBottom: '4px' }}>
            Trader Risk Leaderboard
          </h3>
          <div style={{ fontSize: '11px', color: '#7e7a9c', marginBottom: '16px' }}>
            Calculated by C++ RiskScorer (0–100 risk points)
          </div>

          <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', flex: 1 }}>
            {displayRisks.map((trader) => {
              const isHigh = trader.score >= 50;
              const isMed = trader.score >= 30 && trader.score < 50;
              return (
                <div
                  key={trader.id}
                  style={{
                    padding: '12px 14px',
                    borderRadius: '12px',
                    backgroundColor: 'rgba(255, 255, 255, 0.025)',
                    border: '1px solid rgba(255, 255, 255, 0.04)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    gap: '12px',
                  }}
                >
                  <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
                    <div
                      style={{
                        width: '32px',
                        height: '32px',
                        borderRadius: '8px',
                        backgroundColor: trader.type === 'BOT' ? 'rgba(139, 92, 246, 0.15)' : 'rgba(16, 185, 129, 0.15)',
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                        color: trader.type === 'BOT' ? '#a78bfa' : '#34d399',
                      }}
                    >
                      {trader.type === 'BOT' ? <Bot size={16} /> : <Activity size={16} />}
                    </div>
                    <div>
                      <div style={{ fontSize: '13px', fontWeight: 700, color: '#ffffff' }}>
                        {trader.name}
                      </div>
                      <div style={{ fontSize: '10px', color: '#686484' }}>
                        ID: #{trader.id} • {trader.type} TRADER
                      </div>
                    </div>
                  </div>

                  <div style={{ textAlign: 'right' }}>
                    <div
                      className="mono"
                      style={{
                        fontSize: '15px',
                        fontWeight: 800,
                        color: isHigh ? '#f87171' : isMed ? '#fbbf24' : '#34d399',
                      }}
                    >
                      {trader.score} pts
                    </div>
                    <span
                      className={`pill-badge ${isHigh ? 'pill-red' : isMed ? 'pill-amber' : 'pill-green'}`}
                      style={{ fontSize: '9px', padding: '1px 6px' }}
                    >
                      {trader.tier}
                    </span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>

      {/* Live Alerts Incident Table */}
      <div className="glass-card" style={{ padding: '24px', minWidth: 0 }}>
        {/* Table Filter Header */}
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            flexWrap: 'wrap',
            gap: '12px',
            marginBottom: '18px',
          }}
        >
          <div>
            <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff' }}>
              Detected Alert Incidents
            </h3>
            <div style={{ fontSize: '11px', color: '#7e7a9c' }}>
              Showing {filteredAlerts.length} matching alerts
            </div>
          </div>

          {/* Filter Pills */}
          <div style={{ display: 'flex', gap: '8px' }}>
            <select
              value={severityFilter}
              onChange={(e) => setSeverityFilter(e.target.value)}
              className="input-field"
              style={{ width: 'auto', padding: '6px 12px', fontSize: '12px' }}
            >
              <option value="ALL">All Severities</option>
              <option value="CRITICAL">CRITICAL</option>
              <option value="HIGH">HIGH</option>
              <option value="MEDIUM">MEDIUM</option>
              <option value="LOW">LOW</option>
            </select>

            <select
              value={statusFilter}
              onChange={(e) => setStatusFilter(e.target.value)}
              className="input-field"
              style={{ width: 'auto', padding: '6px 12px', fontSize: '12px' }}
            >
              <option value="ALL">All Statuses</option>
              <option value="OPEN">OPEN Only</option>
              <option value="REVIEWED">REVIEWED Only</option>
            </select>
          </div>
        </div>

        {/* Table */}
        <div className="table-responsive">
          <table className="trade-table">
            <thead>
              <tr>
                <th>Alert ID</th>
                <th>Rule Name</th>
                <th>Trader</th>
                <th>Symbol</th>
                <th>Severity</th>
                <th>Risk Pts</th>
                <th>Status</th>
                <th>Timestamp</th>
                <th style={{ textAlign: 'right' }}>Actions</th>
              </tr>
            </thead>
            <tbody>
              {filteredAlerts.map((alt) => {
                const isOpen = alt.review_status === 'OPEN';
                const isHigh = alt.severity === 'HIGH' || alt.severity === 'CRITICAL';
                return (
                  <tr key={alt.alert_id}>
                    <td className="mono" style={{ color: '#8b87a8' }}>#{alt.alert_id}</td>
                    <td>
                      <span className="mono" style={{ fontWeight: 700, color: '#ffffff', fontSize: '12px' }}>
                        {alt.rule_name}
                      </span>
                      {alt.occurrence_count > 1 && (
                        <span className="pill-badge pill-neutral" style={{ marginLeft: '6px', fontSize: '10px' }}>
                          {alt.occurrence_count}×
                        </span>
                      )}
                    </td>
                    <td>
                      <div style={{ fontWeight: 600, color: '#c4c0e0' }}>{alt.trader_name || `Trader #${alt.trader_id}`}</div>
                    </td>
                    <td style={{ fontWeight: 700, color: '#a78bfa' }}>{alt.symbol || 'N/A'}</td>
                    <td>
                      <span className={`pill-badge ${isHigh ? 'pill-red' : 'pill-amber'}`}>
                        {alt.severity}
                      </span>
                    </td>
                    <td className="mono" style={{ fontWeight: 700, color: '#ffffff' }}>
                      {alt.risk_points}
                    </td>
                    <td>
                      <span className={`pill-badge ${isOpen ? 'pill-red' : 'pill-green'}`}>
                        {alt.review_status}
                      </span>
                    </td>
                    <td style={{ fontSize: '11px', color: '#7e7a9c' }}>
                      {formatRelativeTime(alt.created_at)}
                    </td>
                    <td style={{ textAlign: 'right' }}>
                      <div style={{ display: 'inline-flex', gap: '6px' }}>
                        <button
                          onClick={() => setSelectedAlert(alt)}
                          className="btn btn-sm btn-ghost"
                          title="Inspect details"
                        >
                          <Eye size={12} /> Inspect
                        </button>
                        {isOpen && (
                          <button
                            onClick={() => handleReview(alt.alert_id)}
                            className="btn btn-sm btn-success"
                            title="Mark as reviewed"
                          >
                            <Check size={12} /> Clear
                          </button>
                        )}
                      </div>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      </div>

      {/* Alert Details Modal */}
      {selectedAlert && (
        <div className="modal-overlay">
          <div className="modal-content" style={{ padding: '24px' }}>
            <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '16px' }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
                <ShieldAlert size={18} color="#f87171" />
                <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff' }}>
                  Alert Incident #{selectedAlert.alert_id}
                </h3>
              </div>
              <button
                onClick={() => setSelectedAlert(null)}
                style={{ background: 'none', border: 'none', color: '#8b87a8', cursor: 'pointer' }}
              >
                <X size={18} />
              </button>
            </div>

            <div
              style={{
                backgroundColor: 'rgba(255, 255, 255, 0.03)',
                padding: '16px',
                borderRadius: '12px',
                display: 'flex',
                flexDirection: 'column',
                gap: '12px',
                marginBottom: '20px',
              }}
            >
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Rule Violation</span>
                <span className="mono" style={{ fontWeight: 700, color: '#ffffff' }}>{selectedAlert.rule_name}</span>
              </div>

              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Severity & Risk Points</span>
                <span style={{ fontWeight: 700, color: '#f87171' }}>
                  {selectedAlert.severity} ({selectedAlert.risk_points} pts)
                </span>
              </div>

              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Target Trader</span>
                <span style={{ color: '#c4c0e0' }}>{selectedAlert.trader_name || `Trader #${selectedAlert.trader_id}`}</span>
              </div>

              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Associated Order ID</span>
                <span className="mono" style={{ color: '#c4c0e0' }}>#{selectedAlert.latest_order_id || selectedAlert.order_id || 'N/A'}</span>
              </div>

              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>Episode Occurrences</span>
                <span className="mono" style={{ color: '#a78bfa', fontWeight: 700 }}>{selectedAlert.occurrence_count || 1}×</span>
              </div>

              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px' }}>
                <span style={{ color: '#8b87a8' }}>First Seen / Last Seen</span>
                <span style={{ color: '#c4c0e0', fontSize: '12px' }}>
                  {formatDateTime(selectedAlert.first_seen_at || selectedAlert.created_at)}
                  {selectedAlert.last_seen_at && selectedAlert.last_seen_at !== selectedAlert.first_seen_at && (
                    <> → {formatDateTime(selectedAlert.last_seen_at)}</>
                  )}
                </span>
              </div>

              <div style={{ height: '1px', backgroundColor: 'rgba(255,255,255,0.06)' }} />

              <div>
                <span style={{ color: '#8b87a8', fontSize: '11px', textTransform: 'uppercase', display: 'block', marginBottom: '4px' }}>
                  Detection Analysis
                </span>
                <p style={{ color: '#ffffff', fontSize: '13px', lineHeight: 1.4 }}>
                  {selectedAlert.description}
                </p>
              </div>
            </div>

            <div style={{ display: 'flex', gap: '10px' }}>
              <button
                onClick={() => setSelectedAlert(null)}
                className="btn btn-ghost"
                style={{ flex: 1 }}
              >
                Close
              </button>
              {selectedAlert.review_status === 'OPEN' && (
                <button
                  onClick={() => handleReview(selectedAlert.alert_id)}
                  className="btn btn-success"
                  style={{ flex: 1 }}
                >
                  <Check size={14} /> Clear Alert
                </button>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
