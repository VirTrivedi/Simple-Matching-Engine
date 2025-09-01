#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <sys/socket.h>
#include "../TradeCoreExport/tcp_server_socket.h"
#include "matching_engine_types.h"

// Drop Copy Server Socket
template<typename server_t>
class drop_copy_socket_t : public tcp_server_socket_t {
public:
    server_t* parent_server;
    std::string subscriber_id;
    
    // Constructor
    drop_copy_socket_t(int fd, sockaddr_in clientaddr, socklen_t clientlen,
                      sockaddr_in local_addr, uint16_t local_port_, tcp_server_t* parent)
        : tcp_server_socket_t(fd, clientaddr, clientlen, local_addr, local_port_, parent),
          parent_server(nullptr), subscriber_id("dropcopy_" + std::to_string(fd)) {}
    
    size_t handle_packet(const uint8_t* buf, const size_t len, uint64_t ts, void* sock, bool& should_disconnect) override;
    void gen_shm_name(const int fd, char* buf) override;
    void on_add() override;
    void on_remove() override;
    
    void send_message(const std::string& msg);
};

// Implementation
template<typename server_t>
size_t drop_copy_socket_t<server_t>::handle_packet(const uint8_t*, const size_t len, uint64_t, void*, bool&) {
    return len;
}

template<typename server_t>
void drop_copy_socket_t<server_t>::gen_shm_name(const int fd, char* buf) {
    snprintf(buf, 256, "drop_copy_%d_%d", getpid(), fd);
}

template<typename server_t>
void drop_copy_socket_t<server_t>::on_add() {
    printf("[drop_copy] Subscriber %s connected (fd=%d)\n", subscriber_id.c_str(), get_fd());
}

template<typename server_t>
void drop_copy_socket_t<server_t>::on_remove() {
    printf("[drop_copy] Subscriber %s disconnected (fd=%d)\n", subscriber_id.c_str(), get_fd());
    // Remove from subscribers list
    if (parent_server) {
        auto& subs = parent_server->subscribers;
        subs.erase(std::remove(subs.begin(), subs.end(), this), subs.end());
    }
}

template<typename server_t>
void drop_copy_socket_t<server_t>::send_message(const std::string& msg) {
    ::send(get_fd(), msg.c_str(), msg.length(), 0);
}