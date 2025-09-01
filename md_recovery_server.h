#pragma once
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include "../TradeCoreExport/tcp_server_socket.h"

// Market Data Recovery Server Socket  
template<typename server_t>
class md_recovery_socket_t : public tcp_server_socket_t {
public:
    server_t* parent_server;
    std::string subscriber_id;
    
    // Constructor
    md_recovery_socket_t(int fd, sockaddr_in clientaddr, socklen_t clientlen,
                        sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent)
        : tcp_server_socket_t(fd, clientaddr, clientlen, local_addr, local_port_, parent),
          parent_server(nullptr), subscriber_id("mdrecovery_" + std::to_string(fd)) {}
    
    size_t handle_packet(const uint8_t* buf, const size_t len, uint64_t ts, void* sock, bool& should_disconnect) override;
    void gen_shm_name(const int fd, char* buf) override;
    void on_add() override;
    void on_remove() override;
    
    void send_message(const std::string& msg);
};

// Implementation
template<typename server_t>
size_t md_recovery_socket_t<server_t>::handle_packet(const uint8_t* buf, const size_t len, uint64_t, void*, bool&) {
    std::string request(reinterpret_cast<const char*>(buf), len);
    
    // Remove newline if present
    if (!request.empty() && request.back() == '\n') {
        request.pop_back();
    }
    
    // Parse market data request
    if (request.length() > 9 && request.substr(0, 9) == "SNAPSHOT:" && parent_server) {
        std::string symbol = request.substr(9);
        parent_server->send_snapshot(this, symbol);
    }
    
    return len;
}

template<typename server_t>
void md_recovery_socket_t<server_t>::gen_shm_name(const int fd, char* buf) {
    snprintf(buf, 256, "md_recovery_%d_%d", getpid(), fd);
}

template<typename server_t>
void md_recovery_socket_t<server_t>::on_add() {
    printf("[md_recovery] Subscriber %s connected (fd=%d)\n", subscriber_id.c_str(), get_fd());
}

template<typename server_t>
void md_recovery_socket_t<server_t>::on_remove() {
    printf("[md_recovery] Subscriber %s disconnected (fd=%d)\n", subscriber_id.c_str(), get_fd());
}

template<typename server_t>
void md_recovery_socket_t<server_t>::send_message(const std::string& msg) {
    ::send(get_fd(), msg.c_str(), msg.length(), 0);
}