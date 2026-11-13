import { useContext } from 'react';
import { TradeShieldContext } from './TradeShieldContext';

export function useTradeShield() {
  const ctx = useContext(TradeShieldContext);
  if (!ctx) {
    throw new Error('useTradeShield must be used within a TradeShieldProvider');
  }
  return ctx;
}
