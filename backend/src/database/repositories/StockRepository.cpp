#include "StockRepository.hpp"

namespace tradeshield {

Stock StockRepository::mapRowToStock(Statement& stmt) {
    int64_t id = stmt.getInt64(0);
    std::string symbol = stmt.getText(1);
    std::string company = stmt.getText(2);
    std::string exchange = stmt.getText(3);
    int64_t price = stmt.getInt64(4);
    std::string updated_at = stmt.getText(5);

    return Stock(id, std::move(symbol), std::move(company), std::move(exchange), price, std::move(updated_at));
}

std::optional<Stock> StockRepository::findById(int64_t stock_id) {
    auto stmt = db_.prepare(
        "SELECT stock_id, symbol, company_name, exchange, current_price_paise, updated_at "
        "FROM stocks WHERE stock_id = ?;"
    );
    stmt.bindInt64(1, stock_id);
    if (stmt.step()) {
        return mapRowToStock(stmt);
    }
    return std::nullopt;
}

std::optional<Stock> StockRepository::findBySymbol(const std::string& symbol) {
    auto stmt = db_.prepare(
        "SELECT stock_id, symbol, company_name, exchange, current_price_paise, updated_at "
        "FROM stocks WHERE symbol = ?;"
    );
    stmt.bindText(1, symbol);
    if (stmt.step()) {
        return mapRowToStock(stmt);
    }
    return std::nullopt;
}

std::vector<Stock> StockRepository::findAll() {
    std::vector<Stock> stocks;
    auto stmt = db_.prepare(
        "SELECT stock_id, symbol, company_name, exchange, current_price_paise, updated_at "
        "FROM stocks ORDER BY symbol ASC;"
    );
    while (stmt.step()) {
        stocks.push_back(mapRowToStock(stmt));
    }
    return stocks;
}

int64_t StockRepository::insert(const Stock& stock) {
    auto stmt = db_.prepare(
        "INSERT INTO stocks (symbol, company_name, exchange, current_price_paise, updated_at) "
        "VALUES (?, ?, ?, ?, ?);"
    );
    stmt.bindText(1, stock.getSymbol());
    stmt.bindText(2, stock.getCompanyName());
    stmt.bindText(3, stock.getExchange());
    stmt.bindInt64(4, stock.getCurrentPricePaise());
    stmt.bindText(5, stock.getUpdatedAt());

    stmt.execute();
    return db_.getLastInsertRowId();
}

bool StockRepository::updatePrice(int64_t stock_id, int64_t new_price_paise, const std::string& updated_at) {
    if (new_price_paise <= 0) {
        throw ValidationException("New stock price must be strictly positive");
    }
    auto stmt = db_.prepare(
        "UPDATE stocks SET current_price_paise = ?, updated_at = ? WHERE stock_id = ?;"
    );
    stmt.bindInt64(1, new_price_paise);
    stmt.bindText(2, updated_at);
    stmt.bindInt64(3, stock_id);
    stmt.execute();
    return sqlite3_changes(db_.getRawDb()) > 0;
}

} // namespace tradeshield
