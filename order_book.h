#pragma once
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <unordered_map>
#include "matching_engine_types.h"

// Order Book Implementation
class OrderBook {
private:
    std::string symbol;
    std::map<uint64_t, std::queue<std::shared_ptr<Order>>, std::greater<uint64_t>> bids;
    std::map<uint64_t, std::queue<std::shared_ptr<Order>>> asks;
    std::unordered_map<uint64_t, std::shared_ptr<Order>> order_map;
    uint64_t next_fill_id = 1;
    
public:
    OrderBook(const std::string& sym);
    std::vector<Fill> add_order(std::shared_ptr<Order> order);
    bool cancel_order(uint64_t order_id);
    MarketDataSnapshot get_snapshot() const;
    
private:
    std::vector<Fill> match_order(std::shared_ptr<Order> order);
    void add_to_book(std::shared_ptr<Order> order);
};