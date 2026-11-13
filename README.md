# TradeShield 🛡️

**TradeShield** is a stock market simulator with an integrated real-time fraud and anomaly detection engine ("Shield"). It allows users and automated trading bots to place simulated buy and sell orders on Indian equities (NSE) while continuously monitoring trading activity to detect suspicious behaviors and calculate risk scores.

---

## Key Features
- **Stock Market Simulator**: Real-time quote display and order execution for major Indian stocks (RELIANCE, TCS, INFY, HDFCBANK, TATAMOTORS, ICICIBANK).
- **Portfolio & Ledger Management**: Multi-trader support (human and simulated bots) with integer paise precision to eliminate floating-point rounding errors.
- **Unified Trading Engine**: Every order (human or bot) passes through the same validation, fraud detection, and execution pipeline.
- **Shield Fraud Detection Engine**: Rule-based anomaly detection including:
  - Unusually high order volume
  - Repeated order cancellations (anti-spoofing)
  - High-frequency order bursts
  - Explainable risk points (0–100) and alert categorization
- **Automated Bot Simulation**: Background trading bots running distinct trading strategies (`NORMAL`, `HIGH_VOLUME`, `RAPID_CANCEL`, `RANDOM`).
- **Interactive React Dashboard**: Modern fintech UI with portfolio visualization, live alert feed, and simulation controls.

---

## Tech Stack
- **Frontend**: React 19 + Vite, Recharts, Axios, React Router.
- **Backend**: C++17, Crow micro-framework, SQLite3.
- **Database**: SQLite (WAL mode enabled, strict foreign-key enforcement).

---

## Project Structure
```
TradeShield/
├── backend/            # C++17 Crow REST backend
│   ├── src/            # Models, database layer, trading engine, detection rules, API
│   └── tests/          # C++ unit tests
├── frontend/           # React + Vite frontend application
│   └── src/            # Components, pages, hooks, services, utils
├── database/           # SQLite database schema and seed data
│   ├── schema.sql      # DDL table definitions and indexes
│   └── seed.sql        # Initial instruments, traders, and holdings
├── docs/               # System documentation and schema design
│   ├── architecture.md
│   ├── api_endpoints.md
│   ├── development_plan.md
│   └── TradeShield_Database_Schema.pdf
├── PROJECT_REQUIREMENTS.md
└── AGENTS.md
```

---

## Documentation
- [System Architecture](docs/architecture.md)
- [API Endpoints Specification](docs/api_endpoints.md)
- [Development Plan & Milestones](docs/development_plan.md)
- [Database Schema Specification](docs/TradeShield_Database_Schema.pdf)
