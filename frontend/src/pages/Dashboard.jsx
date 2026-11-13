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
    <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
      {/* Hero Header Section */}
      <div>
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: '8px',
            fontSize: '12px',
            color: '#8b87a8',
            marginBottom: '6px',
            flexWrap: 'wrap',
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
              flexShrink: 0,
            }}
          />
          <span>Last update: {lastUpdateTime ? formatRelativeTime(lastUpdateTime) : 'Just now'}</span>
          <span style={{ color: '#4a4666' }}>•</span>
          <span style={{ color: '#a78bfa' }}>Shield Anomaly Engine Active</span>
        </div>

        <h1
          className="page-title"
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            flexWrap: 'wrap',
          }}
        >
          Live Market Updates 📈
        </h1>
      </div>

      {/* Featured Stock Cards (Responsive Grid) */}
      <div className="responsive-cards-grid">
        {stocks.slice(0, 4).map((stock) => (
          <StockCard
            key={stock.symbol}
            stock={stock}
            onTradeClick={handleTradeStock}
          />
        ))}
      </div>

      {/* Analytics & Operational Row */}
      <div className="responsive-grid-dashboard-middle">
        {/* Left: Portfolio Growth Chart */}
        <div style={{ minWidth: 0 }}>
          <PortfolioChart portfolio={portfolio} />
        </div>

        {/* Right: Shield Alerts & Simulation Quick View */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '20px', minWidth: 0 }}>
          <RecentAlertsWidget alerts={alerts} />
          <SimulationStatusWidget />
        </div>
      </div>

      {/* Market Overview Table */}
      <MarketTable stocks={stocks} onSelectStock={handleTradeStock} />
    </div>
  );
}
