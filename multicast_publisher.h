#pragma once
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include "../TradeCoreExport/udp_client_socket.h"

// Multicast Publisher class
class MulticastPublisher : public udp_client_socket_t {
private:
    std::string multicast_ip;
    uint16_t multicast_port;
    
public:
    // Constructor
    MulticastPublisher(const std::string& ip, uint16_t port, const std::string& bind_ip)
        : udp_client_socket_t(ip, port, bind_ip), multicast_ip(ip), multicast_port(port) {}
    
    void on_add() override {
        printf("[multicast] Publisher started on %s:%d\n", multicast_ip.c_str(), multicast_port);
    }
    
    void on_remove() override {
        printf("[multicast] Publisher stopped\n");
    }
    
    bool handle_packet(const uint8_t*, const size_t, uint64_t) override {
        return true;
    }
    
    void send_message(const std::string& msg) {
        ::send(get_fd(), msg.c_str(), msg.length(), 0);
    }
};