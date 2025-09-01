#include "order_book.h"
#include <algorithm>

OrderBook::OrderBook(const std::string& sym) : symbol(sym) {}

std::vector<Fill> OrderBook::add_order(std::shared_ptr<Order> order) {
    order_map[order->order_id] = order;
    
    if (order->type == OrderType::MARKET) {
        return match_order(order);
    } else {
        auto fills = match_order(order);
        if (order->remaining_quantity > 0) {
            add_to_book(order);
        }
        return fills;
    }
}

std::vector<Fill> OrderBook::match_order(std::shared_ptr<Order> order) {
    std::vector<Fill> fills;
    
    if (order->side == OrderSide::BUY) {
        // Match against asks
        auto it = asks.begin();
        while (it != asks.end() && order->remaining_quantity > 0) {
            if (order->type == OrderType::LIMIT && it->first > order->price) {
                break;
            }
            
            auto& queue = it->second;
            while (!queue.empty() && order->remaining_quantity > 0) {
                auto ask_order = queue.front();
                
                uint64_t trade_qty = std::min(order->remaining_quantity, ask_order->remaining_quantity);
                uint64_t trade_price = ask_order->price;
                
                Fill fill(next_fill_id++, order->order_id, ask_order->order_id, 
                         symbol, trade_qty, trade_price);
                fills.push_back(fill);
                
                order->remaining_quantity -= trade_qty;
                ask_order->remaining_quantity -= trade_qty;
                
                if (ask_order->remaining_quantity == 0) {
                    ask_order->status = OrderStatus::FILLED;
                    queue.pop();
                } else {
                    ask_order->status = OrderStatus::PARTIALLY_FILLED;
                }
            }
            
            if (queue.empty()) {
                it = asks.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // Match against bids
        auto it = bids.begin();
        while (it != bids.end() && order->remaining_quantity > 0) {
            if (order->type == OrderType::LIMIT && it->first < order->price) {
                break;
            }
            
            auto& queue = it->second;
            while (!queue.empty() && order->remaining_quantity > 0) {
                auto bid_order = queue.front();
                
                uint64_t trade_qty = std::min(order->remaining_quantity, bid_order->remaining_quantity);
                uint64_t trade_price = bid_order->price;
                
                Fill fill(next_fill_id++, bid_order->order_id, order->order_id,
                         symbol, trade_qty, trade_price);
                fills.push_back(fill);
                
                order->remaining_quantity -= trade_qty;
                bid_order->remaining_quantity -= trade_qty;
                
                if (bid_order->remaining_quantity == 0) {
                    bid_order->status = OrderStatus::FILLED;
                    queue.pop();
                } else {
                    bid_order->status = OrderStatus::PARTIALLY_FILLED;
                }
            }
            
            if (queue.empty()) {
                it = bids.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Update order status
    if (order->remaining_quantity == 0) {
        order->status = OrderStatus::FILLED;
    } else if (order->remaining_quantity < order->quantity) {
        order->status = OrderStatus::PARTIALLY_FILLED;
    }
    
    return fills;
}

void OrderBook::add_to_book(std::shared_ptr<Order> order) {
    if (order->side == OrderSide::BUY) {
        bids[order->price].push(order);
    } else {
        asks[order->price].push(order);
    }
}

MarketDataSnapshot OrderBook::get_snapshot() const {
    MarketDataSnapshot snapshot;
    snapshot.symbol = symbol;
    snapshot.timestamp = get_current_timestamp();
    
    if (!bids.empty()) {
        auto& best_bid_queue = bids.begin()->second;
        if (!best_bid_queue.empty()) {
            snapshot.bid_price = bids.begin()->first;
            snapshot.bid_quantity = 0;
            auto temp_queue = best_bid_queue;
            while (!temp_queue.empty()) {
                snapshot.bid_quantity += temp_queue.front()->remaining_quantity;
                temp_queue.pop();
            }
        }
    }
    
    if (!asks.empty()) {
        auto& best_ask_queue = asks.begin()->second;
        if (!best_ask_queue.empty()) {
            snapshot.ask_price = asks.begin()->first;
            snapshot.ask_quantity = 0;
            auto temp_queue = best_ask_queue;
            while (!temp_queue.empty()) {
                snapshot.ask_quantity += temp_queue.front()->remaining_quantity;
                temp_queue.pop();
            }
        }
    }
    
    return snapshot;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    auto it = order_map.find(order_id);
    if (it == order_map.end()) {
        return false;
    }
    
    auto order = it->second;
    order->status = OrderStatus::CANCELLED;
    order->remaining_quantity = 0;
    
    // TODO: Remove from bids/asks queues
    
    return true;
}