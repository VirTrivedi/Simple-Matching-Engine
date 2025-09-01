#pragma once
#include <vector>
#include <string>
#include <cstdio>
#include "../TradeCoreExport/tcp_server_socket.h"

class MatchingEngine;

// Order Gateway Server Socket
template<typename server_t>
class order_gateway_socket_t : public tcp_server_socket_t {
public:
    std::vector<uint8_t> rxbuf;
    server_t* parent_server;
    std::string client_id;
    
    // Constructor
    order_gateway_socket_t(int fd, sockaddr_in clientaddr, socklen_t clientlen, 
                          sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent)
        : tcp_server_socket_t(fd, clientaddr, clientlen, local_addr, local_port_, parent),
          parent_server(nullptr), client_id("client_" + std::to_string(fd)) {}
    
    size_t handle_packet(const uint8_t* buf, const size_t len, uint64_t ts, void* sock, bool& should_disconnect) override;
    void gen_shm_name(const int fd, char* buf) override;
    void on_add() override;
    void on_remove() override;
};

// Implementation
template<typename server_t>
size_t order_gateway_socket_t<server_t>::handle_packet(const uint8_t* buf, const size_t len, uint64_t, void*, bool&) {
    std::string message(reinterpret_cast<const char*>(buf), len);
    
    // Remove newline if present
    if (!message.empty() && message.back() == '\n') {
        message.pop_back();
    }
    
    if (!message.empty() && parent_server) {
        parent_server->on_order_request(client_id, message);
    }
    
    return len;
}

template<typename server_t>
void order_gateway_socket_t<server_t>::gen_shm_name(const int fd, char* buf) {
    snprintf(buf, 256, "order_gateway_%d_%d", getpid(), fd);
}

template<typename server_t>
void order_gateway_socket_t<server_t>::on_add() {
    printf("[order_gateway] Client %s connected (fd=%d)\n", client_id.c_str(), get_fd());
}

template<typename server_t>
void order_gateway_socket_t<server_t>::on_remove() {
    printf("[order_gateway] Client %s disconnected (fd=%d)\n", client_id.c_str(), get_fd());
}