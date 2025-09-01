#pragma once
#include <cstdint>
#include <string>
#include <chrono>

// Order types and structures
enum class OrderType : uint8_t {
    MARKET = 1,
    LIMIT = 2
};

enum class OrderSide : uint8_t {
    BUY = 1,
    SELL = 2
};

enum class OrderStatus : uint8_t {
    NEW = 1,
    PARTIALLY_FILLED = 2,
    FILLED = 3,
    CANCELLED = 4,
    REJECTED = 5
};

// Helper function to get current timestamp
inline uint64_t get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

// Helper function to convert dollars to nanos
inline uint64_t dollars_to_nanos(double dollars) {
    return static_cast<uint64_t>(dollars * 1000000000.0);
}

// Helper function to convert nanos to dollars
inline double nanos_to_dollars(uint64_t nanos) {
    return static_cast<double>(nanos) / 1000000000.0;
}

struct Order {
    uint64_t order_id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    uint64_t quantity;
    uint64_t remaining_quantity;
    uint64_t price;
    uint64_t timestamp;
    OrderStatus status;
    std::string client_id;
    
    Order(uint64_t id, const std::string& sym, OrderSide s, OrderType t, 
          uint64_t qty, uint64_t px, const std::string& client)
        : order_id(id), symbol(sym), side(s), type(t), quantity(qty), 
          remaining_quantity(qty), price(px), status(OrderStatus::NEW), 
          client_id(client) {
        timestamp = get_current_timestamp();
    }
};

struct Fill {
    uint64_t fill_id;
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    std::string symbol;
    uint64_t quantity;
    uint64_t price;
    uint64_t timestamp;
    
    Fill(uint64_t id, uint64_t buy_id, uint64_t sell_id, const std::string& sym,
         uint64_t qty, uint64_t px)
        : fill_id(id), buy_order_id(buy_id), sell_order_id(sell_id), 
          symbol(sym), quantity(qty), price(px) {
        timestamp = get_current_timestamp();
    }
};

// Market data structures
struct MarketDataSnapshot {
    std::string symbol;
    uint64_t bid_price = 0;
    uint64_t bid_quantity = 0;
    uint64_t ask_price = 0;
    uint64_t ask_quantity = 0;
    uint64_t last_trade_price = 0;
    uint64_t last_trade_quantity = 0;
    uint64_t timestamp = 0;
};