/**
 * TradeShield Bot Simulation Center Page
 * Control interface for the C++ Bot Simulation Engine:
 * - 4 Bot Strategy Profiles (Normal, Suspicious High-Volume, Rapid Cancel, Random)
 * - Tick interval tuning (250ms - 5000ms)
 * - Lifecycle controls (Start, Stop, Step, Reset)
 * - Metrics telemetry (Ticks, Orders, Executions, Cancellations, Errors)
 */

import { useState } from 'react';
import {
  Bot,
  Play,
  Square,
  RotateCcw,
  Activity,
  AlertTriangle,
  Zap,
} from 'lucide-react';
import { useTradeShield } from '../context/useTradeShield';
import { formatNumberIN } from '../utils/formatters';

export default function Simulation() {
  const {
    simulation,
    startSimulation,
    stopSimulation,
    resetSimulation,
    isSimActionLoading,
  } = useTradeShield();

  const [tickInterval, setTickInterval] = useState(simulation.tick_interval_ms || 2000);

  // Bot Profiles matching C++17 implementations
  const botProfiles = [
    {
      id: 2,
      name: 'AlphaBot',
      role: 'Normal Retail Trader',
      strategy: 'NORMAL',
      desc: 'Submits modest orders (5–10 shares) near current market price. Occasionally cancels ~20% of orders. Compliant trading profile.',
      riskTier: 'LOW',
      riskScore: 0,
      color: '#10b981',
      badgeColor: 'pill-green',
      icon: Activity,
    },
    {
      id: 3,
      name: 'ShadowBot',
      role: 'High-Volume Anomaly',
      strategy: 'HIGH_VOLUME',
      desc: 'Injects abnormally large orders (≥100 shares, ≥₹50,000 value). Rotates buys and sales to sustain high volume bursts without cash starvation.',
      riskTier: 'HIGH',
      riskScore: 65,
      color: '#f59e0b',
      badgeColor: 'pill-amber',
      icon: Zap,
    },
    {
      id: 4,
      name: 'FlashCancel',
      role: 'Rapid Cancellation Bot',
      strategy: 'RAPID_CANCEL',
      desc: 'Submits non-marketable buy orders at 10% discount to remain pending, then rapidly cancels them within seconds to simulate spoofing attacks.',
      riskTier: 'CRITICAL',
      riskScore: 70,
      color: '#ef4444',
      badgeColor: 'pill-red',
      icon: AlertTriangle,
    },
    {
      id: 5,
      name: 'StochasticBot',
      role: 'Randomized Trader',
      strategy: 'RANDOM',
      desc: 'Generates bounded, pseudo-random order prices and quantities using std::mt19937_64 PRNG with deterministic seed capability.',
      riskTier: 'LOW',
      riskScore: 15,
      color: '#8b5cf6',
      badgeColor: 'pill-purple',
      icon: Bot,
    },
  ];

  const handleStart = () => {
    startSimulation(Number(tickInterval));
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Header */}
      <div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '6px', flexWrap: 'wrap' }}>
          <span className="pill-badge pill-purple">C++ BOT ENGINE</span>
          <span style={{ fontSize: '12px', color: '#8b87a8' }}>Multi-Threaded Strategy Coordination</span>
        </div>
        <h1 className="page-title">
          Bot Simulation Center
        </h1>
        <p className="page-subtitle">
          Simulates authentic market participants through the TradingEngine to trigger Shield pattern recognition.
        </p>
      </div>

      {/* Control Banner Card */}
      <div
        className="glass-card"
        style={{
          padding: '24px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          flexWrap: 'wrap',
          gap: '20px',
        }}
      >
        <div style={{ display: 'flex', alignItems: 'center', gap: '16px', flexWrap: 'wrap' }}>
          <div
            style={{
              width: '52px',
              height: '52px',
              borderRadius: '16px',
              background: simulation.is_running
                ? 'linear-gradient(135deg, #10b981 0%, #059669 100%)'
                : 'linear-gradient(135deg, #3730a3 0%, #1e1b4b 100%)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#ffffff',
              boxShadow: simulation.is_running ? '0 0 24px rgba(16, 185, 129, 0.4)' : 'none',
              transition: 'all 0.3s ease',
              flexShrink: 0,
            }}
          >
            <Bot size={26} />
          </div>

          <div>
            <div style={{ display: 'flex', alignItems: 'center', gap: '8px', flexWrap: 'wrap' }}>
              <h3 style={{ fontSize: '20px', fontWeight: 800, color: '#ffffff' }}>
                Engine Status: {simulation.is_running ? 'RUNNING' : 'STOPPED'}
              </h3>
              <span className={`pill-badge ${simulation.is_running ? 'pill-green' : 'pill-neutral'}`}>
                {simulation.is_running && <span className="pulse-indicator" style={{ backgroundColor: '#10b981', width: '6px', height: '6px' }} />}
                {simulation.is_running ? 'Ticking' : 'Idle'}
              </span>
            </div>
            <div style={{ fontSize: '12px', color: '#8b87a8', marginTop: '4px' }}>
              Interval: {tickInterval}ms per tick • 4 Active Worker Strategies
            </div>
          </div>
        </div>

        {/* Action Controls */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '12px', flexWrap: 'wrap' }}>
          {/* Tick Interval Tuning */}
          <div style={{ display: 'flex', alignItems: 'center', gap: '6px', marginRight: '6px' }}>
            <span style={{ fontSize: '11px', color: '#7e7a9c', textTransform: 'uppercase' }}>Tick Pace:</span>
            <select
              value={tickInterval}
              disabled={simulation.is_running}
              onChange={(e) => setTickInterval(Number(e.target.value))}
              className="input-field"
              style={{ width: 'auto', padding: '6px 10px', fontSize: '12px' }}
            >
              <option value="250">250ms (Ultra Fast)</option>
              <option value="500">500ms (Fast)</option>
              <option value="1000">1000ms (1 sec)</option>
              <option value="2000">2000ms (Normal)</option>
              <option value="5000">5000ms (Slow)</option>
            </select>
          </div>

          {simulation.is_running ? (
            <button
              onClick={stopSimulation}
              disabled={isSimActionLoading}
              className="btn btn-danger"
              style={{ padding: '10px 20px', opacity: isSimActionLoading ? 0.7 : 1 }}
            >
              <Square size={16} /> {isSimActionLoading ? 'Stopping...' : 'Stop Simulation'}
            </button>
          ) : (
            <button
              onClick={handleStart}
              disabled={isSimActionLoading}
              className="btn btn-primary"
              style={{ padding: '10px 20px', opacity: isSimActionLoading ? 0.7 : 1 }}
            >
              <Play size={16} /> {isSimActionLoading ? 'Starting...' : 'Start Simulation'}
            </button>
          )}

          <button
            onClick={resetSimulation}
            disabled={isSimActionLoading}
            className="btn btn-ghost"
            title="Reset simulation telemetry counters"
            style={{ padding: '10px 14px', opacity: isSimActionLoading ? 0.5 : 1 }}
          >
            <RotateCcw size={16} /> Reset
          </button>
        </div>
      </div>

      {/* Metrics Row */}
      <div
        style={{
          display: 'grid',
          gridTemplateColumns: 'repeat(auto-fit, minmax(140px, 1fr))',
          gap: '14px',
        }}
      >
        <div className="glass-card" style={{ padding: '18px', textAlign: 'center' }}>
          <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>Total Ticks</div>
          <div className="mono" style={{ fontSize: '24px', fontWeight: 800, color: '#ffffff', marginTop: '4px' }}>
            {formatNumberIN(simulation.total_ticks || 0)}
          </div>
        </div>

        <div className="glass-card" style={{ padding: '18px', textAlign: 'center' }}>
          <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>Orders Generated</div>
          <div className="mono" style={{ fontSize: '24px', fontWeight: 800, color: '#a78bfa', marginTop: '4px' }}>
            {formatNumberIN(simulation.total_orders_generated || 0)}
          </div>
        </div>

        <div className="glass-card" style={{ padding: '18px', textAlign: 'center' }}>
          <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>Trades Executed</div>
          <div className="mono" style={{ fontSize: '24px', fontWeight: 800, color: '#34d399', marginTop: '4px' }}>
            {formatNumberIN(simulation.total_executions_generated || 0)}
          </div>
        </div>

        <div className="glass-card" style={{ padding: '18px', textAlign: 'center' }}>
          <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>Orders Cancelled</div>
          <div className="mono" style={{ fontSize: '24px', fontWeight: 800, color: '#f87171', marginTop: '4px' }}>
            {formatNumberIN(simulation.total_cancellations_generated || 0)}
          </div>
        </div>

        <div className="glass-card" style={{ padding: '18px', textAlign: 'center' }} title="Pre-trade orders rejected by validation rules (e.g. balance or holding bounds)">
          <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase', fontWeight: 600 }}>Pre-Trade Rejections</div>
          <div className="mono" style={{ fontSize: '24px', fontWeight: 800, color: '#f59e0b', marginTop: '4px' }}>
            {formatNumberIN(simulation.total_rejections !== undefined ? simulation.total_rejections : (simulation.total_errors || 0))}
          </div>
          <div style={{ fontSize: '10px', color: '#7e7a9c', marginTop: '2px' }}>
            {simulation.total_system_faults ? `${simulation.total_system_faults} System Faults` : '0 System Faults'}
          </div>
        </div>
      </div>

      {/* 4 Bot Profiles Grid */}
      <div>
        <h3 style={{ fontSize: '18px', fontWeight: 700, color: '#ffffff', marginBottom: '14px' }}>
          Polymorphic Strategy Profiles
        </h3>

        <div
          style={{
            display: 'grid',
            gridTemplateColumns: 'repeat(auto-fit, minmax(260px, 1fr))',
            gap: '16px',
          }}
        >
          {botProfiles.map((bot) => {
            const Icon = bot.icon;
            return (
              <div
                key={bot.id}
                className="glass-card"
                style={{
                  padding: '22px',
                  display: 'flex',
                  flexDirection: 'column',
                  justifyContent: 'space-between',
                  gap: '14px',
                }}
              >
                <div>
                  <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', marginBottom: '12px' }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
                      <div
                        style={{
                          width: '40px',
                          height: '40px',
                          borderRadius: '12px',
                          backgroundColor: `${bot.color}22`,
                          border: `1px solid ${bot.color}44`,
                          display: 'flex',
                          alignItems: 'center',
                          justifyContent: 'center',
                          color: bot.color,
                        }}
                      >
                        <Icon size={20} />
                      </div>
                      <div>
                        <div style={{ fontSize: '16px', fontWeight: 800, color: '#ffffff' }}>
                          {bot.name}
                        </div>
                        <div style={{ fontSize: '11px', color: '#8b87a8' }}>{bot.role}</div>
                      </div>
                    </div>

                    <span className={`pill-badge ${bot.badgeColor}`} style={{ fontSize: '10px' }}>
                      {bot.riskTier} RISK
                    </span>
                  </div>

                  <p style={{ fontSize: '12px', color: '#a7a3c8', lineHeight: 1.4 }}>
                    {bot.desc}
                  </p>
                </div>

                <div
                  style={{
                    paddingTop: '12px',
                    borderTop: '1px solid rgba(255, 255, 255, 0.05)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    fontSize: '11px',
                  }}
                >
                  <span style={{ color: '#7e7a9c' }}>Trader ID: #{bot.id}</span>
                  <span className="mono" style={{ fontWeight: 700, color: bot.color }}>
                    Risk Score: {bot.riskScore} pts
                  </span>
                </div>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}
