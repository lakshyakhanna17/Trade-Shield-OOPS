/**
 * TradeShield React Application Root
 * Sets up BrowserRouter, TradeShieldProvider, Lazy-loaded Page Routes and MainLayout.
 */

import { lazy, Suspense } from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { TradeShieldProvider } from './context/TradeShieldProvider';
import MainLayout from './components/layout/MainLayout';
import ErrorBoundary from './components/common/ErrorBoundary';
import { Shield } from 'lucide-react';

// Code-split pages for performance optimization
const Dashboard = lazy(() => import('./pages/Dashboard'));
const Market = lazy(() => import('./pages/Market'));
const Trading = lazy(() => import('./pages/Trading'));
const Portfolio = lazy(() => import('./pages/Portfolio'));
const ShieldPage = lazy(() => import('./pages/Shield'));
const Simulation = lazy(() => import('./pages/Simulation'));
const Transactions = lazy(() => import('./pages/Transactions'));

function PageLoader() {
  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        minHeight: '60vh',
        gap: '16px',
      }}
    >
      <div
        style={{
          width: '44px',
          height: '44px',
          borderRadius: '14px',
          background: 'linear-gradient(135deg, #7c3aed 0%, #4f46e5 100%)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          color: '#ffffff',
          boxShadow: '0 4px 20px rgba(124, 58, 237, 0.4)',
        }}
      >
        <Shield size={22} className="pulse-indicator" />
      </div>
      <div style={{ fontSize: '13px', color: '#8b87a8', fontWeight: 500 }}>
        Loading TradeShield Module...
      </div>
    </div>
  );
}

export default function App() {
  return (
    <BrowserRouter>
      <ErrorBoundary>
        <TradeShieldProvider>
          <Suspense fallback={<PageLoader />}>
            <Routes>
              <Route path="/" element={<MainLayout />}>
                <Route index element={<Dashboard />} />
                <Route path="market" element={<Market />} />
                <Route path="trading" element={<Trading />} />
                <Route path="portfolio" element={<Portfolio />} />
                <Route path="shield" element={<ShieldPage />} />
                <Route path="simulation" element={<Simulation />} />
                <Route path="transactions" element={<Transactions />} />
                <Route path="*" element={<Navigate to="/" replace />} />
              </Route>
            </Routes>
          </Suspense>
        </TradeShieldProvider>
      </ErrorBoundary>
    </BrowserRouter>
  );
}
