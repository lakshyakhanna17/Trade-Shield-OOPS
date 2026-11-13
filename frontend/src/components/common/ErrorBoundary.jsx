import React from 'react';
import { AlertTriangle, RotateCcw, Home } from 'lucide-react';

/**
 * TradeShield React Error Boundary
 * Catches render-time JavaScript exceptions in child component trees,
 * logs diagnostic metadata, and renders a recoverable dark fintech error UI
 * instead of leaving the user with a blank white/black screen.
 */
export default class ErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { hasError: false, error: null, errorInfo: null };
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error };
  }

  componentDidCatch(error, errorInfo) {
    console.error('TradeShield Uncaught Error Caught by Boundary:', error, errorInfo);
    this.setState({ errorInfo });
  }

  handleReset = () => {
    this.setState({ hasError: false, error: null, errorInfo: null });
    if (this.props.onReset) {
      this.props.onReset();
    }
  };

  handleGoHome = () => {
    window.location.href = '/';
  };

  render() {
    if (this.state.hasError) {
      if (this.props.fallback) {
        return this.props.fallback;
      }

      return (
        <div
          style={{
            minHeight: this.props.compact ? '240px' : '50vh',
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center',
            padding: '32px 20px',
            textAlign: 'center',
          }}
        >
          <div
            className="glass-card"
            style={{
              maxWidth: '560px',
              width: '100%',
              padding: '32px',
              backgroundColor: '#12101b',
              border: '1px solid rgba(239, 68, 68, 0.25)',
              boxShadow: '0 16px 48px rgba(0, 0, 0, 0.6), 0 0 24px rgba(239, 68, 68, 0.1)',
            }}
          >
            <div
              style={{
                width: '56px',
                height: '56px',
                borderRadius: '16px',
                backgroundColor: 'rgba(239, 68, 68, 0.15)',
                border: '1px solid rgba(239, 68, 68, 0.3)',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: '#f87171',
                margin: '0 auto 18px',
              }}
            >
              <AlertTriangle size={28} />
            </div>

            <h2 style={{ fontSize: '20px', fontWeight: 800, color: '#ffffff', marginBottom: '8px' }}>
              View Rendering Interrupted
            </h2>

            <p style={{ fontSize: '13px', color: '#9d99b8', lineHeight: 1.5, marginBottom: '20px' }}>
              An unexpected condition occurred while updating TradeShield real-time views. The error has been contained to prevent application termination.
            </p>

            {this.state.error && (
              <div
                className="mono"
                style={{
                  fontSize: '11px',
                  color: '#f87171',
                  backgroundColor: 'rgba(0, 0, 0, 0.4)',
                  padding: '10px 14px',
                  borderRadius: '8px',
                  border: '1px solid rgba(239, 68, 68, 0.2)',
                  marginBottom: '24px',
                  wordBreak: 'break-all',
                  textAlign: 'left',
                  maxHeight: '120px',
                  overflowY: 'auto',
                }}
              >
                {this.state.error.toString()}
              </div>
            )}

            <div style={{ display: 'flex', gap: '12px', justifyContent: 'center' }}>
              <button
                onClick={this.handleReset}
                className="btn btn-primary"
                style={{ padding: '10px 20px', fontSize: '13px' }}
              >
                <RotateCcw size={15} /> Try Again
              </button>
              <button
                onClick={this.handleGoHome}
                className="btn btn-ghost"
                style={{ padding: '10px 18px', fontSize: '13px' }}
              >
                <Home size={15} /> Dashboard
              </button>
            </div>
          </div>
        </div>
      );
    }

    return this.props.children;
  }
}
