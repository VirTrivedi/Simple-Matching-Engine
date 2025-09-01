#include <cstdio>
#include <signal.h>
#include <string>
#include "../TradeCoreExport/event_manager.h"
#include "matching_engine.h"

// Global event manager
event_manager_t* g_event_manager = nullptr;

// Signal handler for CTRL-C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        if (g_event_manager) {
            printf("\n[matching_engine] Shutting down gracefully...\n");
            g_event_manager->shutdown();
        } else {
            exit(0);
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: %s <bind_ip> <multicast_ip> <multicast_port>\n", argv[0]);
        printf("Example: %s 192.168.1.100 239.255.0.1 9999\n", argv[0]);
        return 1;
    }
    
    std::string bind_ip = argv[1];
    std::string multicast_ip = argv[2];
    uint16_t multicast_port = std::atoi(argv[3]);
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    printf("[matching_engine] Initializing matching engine...\n");
    
    event_manager_t em;
    g_event_manager = &em;
    
    MatchingEngine engine(bind_ip, multicast_ip, multicast_port);
    engine.start(&em);
    
    printf("[matching_engine] Press CTRL-C to shutdown gracefully\n");
    printf("[matching_engine] Order format: BUY:SYMBOL:QUANTITY:PRICE_NANOS\n");
    printf("[matching_engine]   Example: BUY:AAPL:100:150123456789 (for $150.123456789)\n");
    printf("[matching_engine] MD Recovery format: SNAPSHOT:SYMBOL (e.g., SNAPSHOT:AAPL)\n");
    printf("[matching_engine] Note: Prices are in nanos for maximum precision\n");
    
    em.run();
    
    printf("[matching_engine] Shutdown complete\n");
    return 0;
}