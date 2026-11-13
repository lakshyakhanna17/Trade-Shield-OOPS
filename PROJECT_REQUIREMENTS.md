# TradeShield — Project Requirements

## 1. Project Overview
TradeShield is a stock market simulator with an integrated fraud and anomaly detection system.

Users and simulated bots can place buy and sell orders. The system monitors trading activity and detects suspicious behaviour.

## 2. Main Features
- Stock market dashboard
- Trader and portfolio management
- Buy and sell order simulation
- Automated trading bots
- Fraud and anomaly detection
- Risk scoring system
- Alerts and monitoring dashboard
- Order history and trader activity

## 3. Trading Bots
Implement:
- Normal Trader
- Suspicious Trader
- Random Trader

All bots must generate actual simulated order events through the same trading pipeline used by manual traders.

## 4. Fraud Detection
Implement separate rules for:
- Unusually high order volume
- Repeated order cancellations
- Unusually high order frequency

Each rule should calculate risk points and provide an explanation.

A large order alone must not automatically be classified as fraud.

## 5. OOP Requirements
Demonstrate:
- Encapsulation
- Inheritance
- Abstraction
- Polymorphism

Use the Strategy Design Pattern for bot behaviour where appropriate.

## 6. Technology Stack
Frontend: React + Vite
Frontend language: JavaScript
Backend: C++17 + Crow
Database: SQLite
Charts: Recharts
API communication: Axios

## 7. Important Rules
- Follow the supplied database schema.
- Validate every order.
- Prevent traders from selling shares they do not own.
- Store financial values using integer paise.
- Keep market prices separate from simulated transactions.
- Do not execute real stock market transactions.
- Use modular, maintainable code.
- Include tests and error handling.