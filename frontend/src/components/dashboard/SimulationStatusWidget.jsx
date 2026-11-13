/**
 * TradeShield Simulation Quick Controller Widget
 * Allows starting/stopping bot trading directly from the dashboard.
 */

import { Play, Square, Bot, RotateCcw } from 'lucide-react';
import { useTradeShield } from '../../context/useTradeShield';

export default function SimulationStatusWidget() {
  const { simulation, startSimulation, stopSimulation, resetSimulation, isSimActionLoading } = useTradeShield();

  return (
    <div className="glass-card" style={{ padding: '22px 22px 18px', height: '100%', display: 'flex', flexDirection: 'column', justifyContent: 'space-between', minWidth: 0 }}>
      {/* Top Header */}
      <div>
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '14px' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
            <div
              style={{
                width: '28px',
                height: '28px',
                borderRadius: '8px',
                backgroundColor: 'rgba(139, 92, 246, 0.15)',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: '#a78bfa',
              }}
            >
              <Bot size={16} />
            </div>
            <div>
              <h4 style={{ fontSize: '15px', fontWeight: 700, color: '#ffffff' }}>Bot Simulation Engine</h4>
              <div style={{ fontSize: '11px', color: '#7e7a9c' }}>4 Automated Strategies Active</div>
            </div>
          </div>

          <span className={`pill-badge ${simulation.is_running ? 'pill-purple' : 'pill-neutral'}`}>
            <span
              className="pulse-indicator"
              style={{
                backgroundColor: simulation.is_running ? '#a78bfa' : '#686484',
                width: '6px',
                height: '6px',
              }}
            />
            {simulation.is_running ? 'RUNNING' : 'STOPPED'}
          </span>
        </div>

        {/* Quick Stats Grid */}
        <div
          style={{
            display: 'grid',
            gridTemplateColumns: 'repeat(3, 1fr)',
            gap: '8px',
            marginBottom: '16px',
          }}
        >
          <div
            style={{
              padding: '10px',
              backgroundColor: 'rgba(255, 255, 255, 0.02)',
              borderRadius: '10px',
              border: '1px solid rgba(255, 255, 255, 0.04)',
              textAlign: 'center',
            }}
          >
            <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>Ticks</div>
            <div className="mono" style={{ fontSize: '16px', fontWeight: 700, color: '#ffffff', marginTop: '2px' }}>
              {simulation.total_ticks || 0}
            </div>
          </div>

          <div
            style={{
              padding: '10px',
              backgroundColor: 'rgba(255, 255, 255, 0.02)',
              borderRadius: '10px',
              border: '1px solid rgba(255, 255, 255, 0.04)',
              textAlign: 'center',
            }}
          >
            <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>Orders</div>
            <div className="mono" style={{ fontSize: '16px', fontWeight: 700, color: '#a78bfa', marginTop: '2px' }}>
              {simulation.total_orders_generated || 0}
            </div>
          </div>

          <div
            style={{
              padding: '10px',
              backgroundColor: 'rgba(255, 255, 255, 0.02)',
              borderRadius: '10px',
              border: '1px solid rgba(255, 255, 255, 0.04)',
              textAlign: 'center',
            }}
          >
            <div style={{ fontSize: '10px', color: '#7e7a9c', textTransform: 'uppercase' }}>Cancels</div>
            <div className="mono" style={{ fontSize: '16px', fontWeight: 700, color: '#f87171', marginTop: '2px' }}>
              {simulation.total_cancellations_generated || 0}
            </div>
          </div>
        </div>
      </div>

      {/* Buttons */}
      <div style={{ display: 'flex', gap: '8px' }}>
        {simulation.is_running ? (
          <button
            onClick={stopSimulation}
            disabled={isSimActionLoading}
            className="btn btn-danger"
            style={{ flex: 1, padding: '10px', fontSize: '12px', opacity: isSimActionLoading ? 0.7 : 1 }}
          >
            <Square size={14} /> {isSimActionLoading ? 'Stopping...' : 'Stop Simulation'}
          </button>
        ) : (
          <button
            onClick={() => startSimulation(2000)}
            disabled={isSimActionLoading}
            className="btn btn-primary"
            style={{ flex: 1, padding: '10px', fontSize: '12px', opacity: isSimActionLoading ? 0.7 : 1 }}
          >
            <Play size={14} /> {isSimActionLoading ? 'Starting...' : 'Start Bot Trading'}
          </button>
        )}

        <button
          onClick={resetSimulation}
          disabled={isSimActionLoading}
          className="btn btn-ghost"
          title="Reset metrics"
          style={{ padding: '10px', width: '38px', opacity: isSimActionLoading ? 0.5 : 1 }}
        >
          <RotateCcw size={14} />
        </button>
      </div>
    </div>
  );
}
