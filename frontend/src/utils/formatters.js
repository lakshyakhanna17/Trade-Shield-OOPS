/**
 * TradeShield Financial & Date Formatters
 * Formats integer paise into INR currency strings and percentages.
 */

// Formats paise into Indian Rupee string (e.g. 245000 -> ₹2,450.00)
export function formatPaiseToINR(paise, includeDecimals = true) {
  if (paise === null || paise === undefined || isNaN(paise)) return '₹0.00';
  const rupees = Number(paise) / 100;
  return new Intl.NumberFormat('en-IN', {
    style: 'currency',
    currency: 'INR',
    minimumFractionDigits: includeDecimals ? 2 : 0,
    maximumFractionDigits: includeDecimals ? 2 : 0,
  }).format(rupees);
}

// Formats a float rupee value (e.g. 2450.5 -> ₹2,450.50)
export function formatRupees(rupees, includeDecimals = true) {
  if (rupees === null || rupees === undefined || isNaN(rupees)) return '₹0.00';
  return new Intl.NumberFormat('en-IN', {
    style: 'currency',
    currency: 'INR',
    minimumFractionDigits: includeDecimals ? 2 : 0,
    maximumFractionDigits: includeDecimals ? 2 : 0,
  }).format(rupees);
}

// Formats percentage with sign (e.g. 1.24 -> +1.24%, -0.82 -> -0.82%)
export function formatPercent(val, decimals = 2) {
  if (val === null || val === undefined || isNaN(val)) return '0.00%';
  const num = Number(val);
  const sign = num > 0 ? '+' : '';
  return `${sign}${num.toFixed(decimals)}%`;
}

// Formats number with Indian numbering system (e.g. 1500000 -> 15,00,000)
export function formatNumberIN(num) {
  if (num === null || num === undefined || isNaN(num)) return '0';
  return new Intl.NumberFormat('en-IN').format(num);
}

// Formats ISO timestamp to readable date/time (e.g. 2026-11-11T06:26:41Z -> 11 Nov 2026, 11:56 AM)
export function formatDateTime(isoString) {
  if (!isoString) return '—';
  try {
    const d = new Date(isoString);
    if (isNaN(d.getTime())) return isoString;
    return d.toLocaleDateString('en-IN', {
      day: '2-digit',
      month: 'short',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      hour12: true,
    });
  } catch {
    return isoString;
  }
}

// Formats ISO timestamp to relative time (e.g. "2 min ago", "Just now")
export function formatRelativeTime(isoString) {
  if (!isoString) return '—';
  try {
    const d = new Date(isoString);
    const now = new Date();
    const diffSec = Math.floor((now - d) / 1000);
    if (diffSec < 10) return 'Just now';
    if (diffSec < 60) return `${diffSec}s ago`;
    const diffMin = Math.floor(diffSec / 60);
    if (diffMin < 60) return `${diffMin} min ago`;
    const diffHour = Math.floor(diffMin / 60);
    if (diffHour < 24) return `${diffHour}h ago`;
    return d.toLocaleDateString('en-IN', { day: '2-digit', month: 'short' });
  } catch {
    return isoString;
  }
}
