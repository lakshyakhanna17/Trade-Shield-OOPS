/**
 * TradeShield Dashboard Page
 * Primary landing page faithful to the reference screenshot design:
 * 1. Status subtitle (Last update: 2 min ago)
 * 2. Hero title (Live Stock Updates 📈)
 * 3. Featured Stock Cards row
 * 4. Middle Analytics row (Portfolio Chart + Alerts + Bot Controller)
 * 5. Market Overview Table
 */

import { useNavigate } from 'react-router-dom';
import { useTradeShield } from '../context/useTradeShield';
import { formatRelativeTime } from '../utils/formatters';
import StockCard from '../components/dashboard/StockCard';
import MarketTable from '../components/dashboard/MarketTable';
import PortfolioChart from '../components/dashboard/PortfolioChart';
import RecentAlertsWidget from '../components/dashboard/RecentAlertsWidget';
import SimulationStatusWidget from '../components/dashboard/SimulationStatusWidget';

export default function Dashboard() {
  const navigate = useNavigate();
  const { stocks, portfolio, alerts, lastUpdateTime } = useTradeShield();

  const handleTradeStock = (stock) => {
    navigate('/trading', { state: { preselectStock: stock.symbol } });
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '28px' }}>
      {/* Hero Header Section (Directly from reference screenshot) */}
      <div>
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: '8px',
            fontSize: '12px',
            color: '#8b87a8',
            marginBottom: '6px',
          }}
        >
          <span
            style={{
              width: '7px',
              height: '7px',
              borderRadius: '50%',
              backgroundColor: '#10b981',
              boxShadow: '0 0 8px #10b981',
              display: 'inline-block',
            }}
          />
          <span>Last update: {lastUpdateTime ? formatRelativeTime(lastUpdateTime) : 'Just now'}</span>
          <span style={{ color: '#4a4666' }}>•</span>
          <span style={{ color: '#a78bfa' }}>Shield Anomaly Engine Active</span>
        </div>

        <h1
          style={{
            fontSize: '34px',
            fontWeight: 800,
            letterSpacing: '-0.03em',
            color: '#ffffff',
            display: 'flex',
            alignItems: 'center',
            gap: '12px',
          }}
        >
          Live Market Updates 📈
        </h1>
      </div>

      {/* Featured Stock Cards (Grid matching screenshot's top card row) */}
      <div
        style={{
          display: 'grid',
          gridTemplateColumns: 'repeat(auto-fit, minmax(280px, 1fr))',
          gap: '20px',
        }}
      >
        {stocks.slice(0, 4).map((stock) => (
          <StockCard
            key={stock.symbol}
            stock={stock}
            onTradeClick={handleTradeStock}
          />
        ))}
      </div>

      {/* Analytics & Operational Row */}
      <div
        style={{
          display: 'grid',
          gridTemplateColumns: 'repeat(auto-fit, minmax(360px, 1fr))',
          gap: '20px',
        }}
      >
        {/* Left: Portfolio Growth Chart */}
        <div style={{ gridColumn: 'span 2' }}>
          <PortfolioChart portfolio={portfolio} />
        </div>

        {/* Right: Shield Alerts & Simulation Quick View */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '20px' }}>
          <RecentAlertsWidget alerts={alerts} />
          <SimulationStatusWidget />
        </div>
      </div>

      {/* Market Overview Table (From bottom half of screenshot) */}
      <MarketTable stocks={stocks} onSelectStock={handleTradeStock} />
    </div>
  );
}
