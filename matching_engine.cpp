#include "matching_engine.h"
#include <cstdio>

// MatchingEngine Constructor
MatchingEngine::MatchingEngine(const std::string& bind_ip_param, 
                               const std::string& mcast_ip, uint16_t mcast_port)
    : bind_ip(bind_ip_param), multicast_ip(mcast_ip), multicast_port(mcast_port) {
    
    // Create servers
    order_gateway = std::make_unique<OrderGatewayServer>(this, order_gateway_port, bind_ip);
    drop_copy_server = std::make_unique<DropCopyServer>(this, drop_copy_port, bind_ip);
    md_recovery_server = std::make_unique<MDRecoveryServer>(this, md_recovery_port, bind_ip);
    
    // Create multicast publisher
    multicast_publisher = std::make_unique<MulticastPublisher>(mcast_ip, mcast_port, bind_ip);
    
    // Initialize some test symbols
    order_books["AAPL"] = std::make_unique<OrderBook>("AAPL");
    order_books["MSFT"] = std::make_unique<OrderBook>("MSFT");
    order_books["TSLA"] = std::make_unique<OrderBook>("TSLA");
}

void MatchingEngine::start(event_manager_t* em) {
    // Add all servers to event manager
    em->add_pollable(order_gateway.get());
    em->add_pollable(drop_copy_server.get());
    em->add_pollable(md_recovery_server.get());
    em->add_pollable(multicast_publisher.get());
    
    printf("[matching_engine] Started on %s\n", bind_ip.c_str());
    printf("[matching_engine] Order Gateway:     port %d\n", order_gateway_port);
    printf("[matching_engine] Drop Copy:         port %d\n", drop_copy_port);
    printf("[matching_engine] Market Data:       port %d\n", md_recovery_port);
    printf("[matching_engine] Multicast:         %s:%d\n", multicast_ip.c_str(), multicast_port);
}

void MatchingEngine::process_order_request(const std::string& client_id, const std::string& order_msg) {
    printf("[matching_engine] Order from %s: %s\n", client_id.c_str(), order_msg.c_str());
    
    // Parse order message
    std::vector<std::string> parts;
    std::stringstream ss(order_msg);
    std::string part;
    while (std::getline(ss, part, ':')) {
        parts.push_back(part);
    }
    
    if (parts.size() != 4) {
        printf("[matching_engine] Invalid order format from %s\n", client_id.c_str());
        return;
    }
    
    OrderSide side = (parts[0] == "BUY") ? OrderSide::BUY : OrderSide::SELL;
    std::string symbol = parts[1];
    uint64_t quantity = std::stoull(parts[2]);
    uint64_t price_nanos = std::stoull(parts[3]);
    
    // Log order with dollar conversion
    printf("[matching_engine] Processing %s %lu %s at $%.9f (%lu nanos)\n", 
           (side == OrderSide::BUY) ? "BUY" : "SELL", 
           quantity, symbol.c_str(), 
           nanos_to_dollars(price_nanos), price_nanos);
    
    if (order_books.find(symbol) == order_books.end()) {
        order_books[symbol] = std::make_unique<OrderBook>(symbol);
    }
    
    auto order = std::make_shared<Order>(next_order_id++, symbol, side, OrderType::LIMIT, quantity, price_nanos, client_id);
    auto fills = order_books[symbol]->add_order(order);
    
    // Broadcast order update
    drop_copy_server->broadcast_order_update(*order);
    
    // Broadcast fills
    for (const auto& fill : fills) {
        printf("[matching_engine] Fill: %lu shares at $%.9f (%lu nanos)\n", 
               fill.quantity, nanos_to_dollars(fill.price), fill.price);
        drop_copy_server->broadcast_fill(fill);
    }
    
    // Publish market data
    auto snapshot = order_books[symbol]->get_snapshot();
    publish_market_data(snapshot);
}

void MatchingEngine::send_market_data_snapshot(md_recovery_socket_t<MDRecoveryServer>* client, const std::string& symbol) {
    if (order_books.find(symbol) != order_books.end()) {
        auto snapshot = order_books[symbol]->get_snapshot();
        
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), 
                "SNAPSHOT:%s:BID:%lu@%lu($%.9f):ASK:%lu@%lu($%.9f):LAST:%lu@%lu($%.9f)\n",
                symbol.c_str(),
                snapshot.bid_quantity, snapshot.bid_price, nanos_to_dollars(snapshot.bid_price),
                snapshot.ask_quantity, snapshot.ask_price, nanos_to_dollars(snapshot.ask_price),
                snapshot.last_trade_quantity, snapshot.last_trade_price, nanos_to_dollars(snapshot.last_trade_price));
        
        client->send_message(std::string(buffer));
    }
}

void MatchingEngine::publish_market_data(const MarketDataSnapshot& snapshot) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
            "MD:%s:BID:%lu@%lu($%.9f):ASK:%lu@%lu($%.9f):LAST:%lu@%lu($%.9f):TS:%lu\n",
            snapshot.symbol.c_str(),
            snapshot.bid_quantity, snapshot.bid_price, nanos_to_dollars(snapshot.bid_price),
            snapshot.ask_quantity, snapshot.ask_price, nanos_to_dollars(snapshot.ask_price),
            snapshot.last_trade_quantity, snapshot.last_trade_price, nanos_to_dollars(snapshot.last_trade_price),
            snapshot.timestamp);
    
    std::string msg(buffer);
    multicast_publisher->send_message(msg);
}

// OrderGatewayServer Implementation
MatchingEngine::OrderGatewayServer::OrderGatewayServer(MatchingEngine* eng, uint16_t port, const std::string& ip) 
    : tcp_server_t(port, ip), engine(eng) {}

tcp_server_socket_t* MatchingEngine::OrderGatewayServer::make_child(int fd, sockaddr_in clientaddr, socklen_t clientlen, 
                                                                   sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent) {
    auto* socket = new order_gateway_socket_t<OrderGatewayServer>(fd, clientaddr, clientlen, local_addr, local_port_, parent);
    socket->parent_server = this;
    return socket;
}

void MatchingEngine::OrderGatewayServer::emplace_reserve(std::vector<std::pair<tcp_server_socket_t*, uint8_t*>>&, const uint64_t) {
    // TODO
}

void MatchingEngine::OrderGatewayServer::on_add() {
    printf("[order_gateway_server] Server started on port %d\n", engine->order_gateway_port);
}

void MatchingEngine::OrderGatewayServer::on_remove() {
    printf("[order_gateway_server] Server stopped\n");
}

void MatchingEngine::OrderGatewayServer::on_order_request(const std::string& client_id, const std::string& order_msg) {
    engine->process_order_request(client_id, order_msg);
}

// DropCopyServer Implementation
MatchingEngine::DropCopyServer::DropCopyServer(MatchingEngine* eng, uint16_t port, const std::string& ip)
    : tcp_server_t(port, ip), engine(eng) {}

tcp_server_socket_t* MatchingEngine::DropCopyServer::make_child(int fd, sockaddr_in clientaddr, socklen_t clientlen,
                                                               sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent) {
    auto* socket = new drop_copy_socket_t<DropCopyServer>(fd, clientaddr, clientlen, local_addr, local_port_, parent);
    socket->parent_server = this;
    subscribers.push_back(socket);
    return socket;
}

void MatchingEngine::DropCopyServer::emplace_reserve(std::vector<std::pair<tcp_server_socket_t*, uint8_t*>>&, const uint64_t) {
    // TODO
}

void MatchingEngine::DropCopyServer::on_add() {
    printf("[drop_copy_server] Server started on port %d\n", engine->drop_copy_port);
}

void MatchingEngine::DropCopyServer::on_remove() {
    printf("[drop_copy_server] Server stopped\n");
}

void MatchingEngine::DropCopyServer::broadcast_fill(const Fill& fill) {
    std::string fill_msg = format_fill_message(fill);
    for (auto* subscriber : subscribers) {
        subscriber->send_message(fill_msg);
    }
}

void MatchingEngine::DropCopyServer::broadcast_order_update(const Order& order) {
    std::string order_msg = format_order_message(order);
    for (auto* subscriber : subscribers) {
        subscriber->send_message(order_msg);
    }
}

std::string MatchingEngine::DropCopyServer::format_fill_message(const Fill& fill) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
            "FILL:%lu:BUY_ORDER:%lu:SELL_ORDER:%lu:SYMBOL:%s:QTY:%lu:PRICE:%lu($%.9f):TS:%lu\n",
            fill.fill_id, fill.buy_order_id, fill.sell_order_id,
            fill.symbol.c_str(), fill.quantity, fill.price, nanos_to_dollars(fill.price), fill.timestamp);
    return std::string(buffer);
}

std::string MatchingEngine::DropCopyServer::format_order_message(const Order& order) {
    char buffer[1024];
    const char* status_str = "";
    switch (order.status) {
        case OrderStatus::NEW: status_str = "NEW"; break;
        case OrderStatus::PARTIALLY_FILLED: status_str = "PARTIAL"; break;
        case OrderStatus::FILLED: status_str = "FILLED"; break;
        case OrderStatus::CANCELLED: status_str = "CANCELLED"; break;
        case OrderStatus::REJECTED: status_str = "REJECTED"; break;
    }
    
    snprintf(buffer, sizeof(buffer),
            "ORDER:%lu:CLIENT:%s:SIDE:%s:SYMBOL:%s:QTY:%lu:REMAINING:%lu:PRICE:%lu($%.9f):STATUS:%s:TS:%lu\n",
            order.order_id, order.client_id.c_str(),
            (order.side == OrderSide::BUY) ? "BUY" : "SELL",
            order.symbol.c_str(), order.quantity, order.remaining_quantity,
            order.price, nanos_to_dollars(order.price), status_str, order.timestamp);
    return std::string(buffer);
}

// MDRecoveryServer Implementation
MatchingEngine::MDRecoveryServer::MDRecoveryServer(MatchingEngine* eng, uint16_t port, const std::string& ip)
    : tcp_server_t(port, ip), engine(eng) {}

tcp_server_socket_t* MatchingEngine::MDRecoveryServer::make_child(int fd, sockaddr_in clientaddr, socklen_t clientlen,
                                                                 sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent) {
    auto* socket = new md_recovery_socket_t<MDRecoveryServer>(fd, clientaddr, clientlen, local_addr, local_port_, parent);
    socket->parent_server = this;
    return socket;
}

void MatchingEngine::MDRecoveryServer::emplace_reserve(std::vector<std::pair<tcp_server_socket_t*, uint8_t*>>&, const uint64_t) {
    // TODO
}

void MatchingEngine::MDRecoveryServer::on_add() {
    printf("[md_recovery_server] Server started on port %d\n", engine->md_recovery_port);
}

void MatchingEngine::MDRecoveryServer::on_remove() {
    printf("[md_recovery_server] Server stopped\n");
}

void MatchingEngine::MDRecoveryServer::send_snapshot(md_recovery_socket_t<MDRecoveryServer>* client, const std::string& symbol) {
    engine->send_market_data_snapshot(client, symbol);
}