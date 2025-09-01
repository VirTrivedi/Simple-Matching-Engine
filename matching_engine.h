#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include "../TradeCoreExport/tcp_server_socket.h"
#include "../TradeCoreExport/event_manager.h"
#include "matching_engine_types.h"
#include "order_book.h"
#include "order_gateway_server.h"
#include "drop_copy_server.h"
#include "md_recovery_server.h"
#include "multicast_publisher.h"

// Main Matching Engine class
class MatchingEngine {
public:
    // Order Gateway Server
    class OrderGatewayServer : public tcp_server_t {
        MatchingEngine* engine;
    public:
        OrderGatewayServer(MatchingEngine* eng, uint16_t port, const std::string& ip);
        
        tcp_server_socket_t* make_child(int fd, sockaddr_in clientaddr, socklen_t clientlen, 
                                       sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent) override;
        
        void emplace_reserve(std::vector<std::pair<tcp_server_socket_t*, uint8_t*>>& socketBuf, const uint64_t len) override;
        void on_add() override;
        void on_remove() override;
        
        void on_order_request(const std::string& client_id, const std::string& order_msg);
    };
    
    // Drop Copy Server
    class DropCopyServer : public tcp_server_t {
        MatchingEngine* engine;
    public:
        std::vector<drop_copy_socket_t<DropCopyServer>*> subscribers;
        
        DropCopyServer(MatchingEngine* eng, uint16_t port, const std::string& ip);
        
        tcp_server_socket_t* make_child(int fd, sockaddr_in clientaddr, socklen_t clientlen,
                                       sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent) override;
        
        void emplace_reserve(std::vector<std::pair<tcp_server_socket_t*, uint8_t*>>& socketBuf, const uint64_t len) override;
        void on_add() override;
        void on_remove() override;
        
        void broadcast_fill(const Fill& fill);
        void broadcast_order_update(const Order& order);
        
    private:
        std::string format_fill_message(const Fill& fill);
        std::string format_order_message(const Order& order);
    };
    
    // Market Data Recovery Server
    class MDRecoveryServer : public tcp_server_t {
        MatchingEngine* engine;
    public:
        MDRecoveryServer(MatchingEngine* eng, uint16_t port, const std::string& ip);
        
        tcp_server_socket_t* make_child(int fd, sockaddr_in clientaddr, socklen_t clientlen,
                                       sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent) override;
        
        void emplace_reserve(std::vector<std::pair<tcp_server_socket_t*, uint8_t*>>& socketBuf, const uint64_t len) override;
        void on_add() override;
        void on_remove() override;
        
        void send_snapshot(md_recovery_socket_t<MDRecoveryServer>* client, const std::string& symbol);
    };

private:
    std::string bind_ip;
    uint16_t order_gateway_port = 8001;
    uint16_t drop_copy_port = 8002;
    uint16_t md_recovery_port = 8003;
    
    std::unique_ptr<OrderGatewayServer> order_gateway;
    std::unique_ptr<DropCopyServer> drop_copy_server;
    std::unique_ptr<MDRecoveryServer> md_recovery_server;
    
    // Order books by symbol
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> order_books;
    
    // Multicast publisher for market data
    std::unique_ptr<MulticastPublisher> multicast_publisher;
    std::string multicast_ip;
    uint16_t multicast_port;
    
    uint64_t next_order_id = 1;
    
public:
    MatchingEngine(const std::string& bind_ip, 
                   const std::string& mcast_ip, uint16_t mcast_port);
    
    void start(event_manager_t* em);
    void process_order_request(const std::string& client_id, const std::string& order_msg);
    void send_market_data_snapshot(md_recovery_socket_t<MDRecoveryServer>* client, const std::string& symbol);
    void publish_market_data(const MarketDataSnapshot& snapshot);
    
    const std::string& get_bind_ip() const { return bind_ip; }
};