#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <array>
#include <memory>
#include <iostream>
#include <iomanip>
#include "feed_network.h"

int sock_fd;
int reuse {1};
sockaddr_in addr {};


void setup_socket() {

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(MULTICAST_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock_fd, (sockaddr*)&addr, sizeof(addr));

    ip_mreq group{};
    inet_pton(AF_INET, MULTICAST_IP_ADDR, &group.imr_multiaddr);
    inet_pton(AF_INET, "127.0.0.1", &group.imr_interface.s_addr);
    setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));
}


// maps an ITCH 5.0 message type byte to a human-readable name (milestone 1 visibility only)
static const char* itch_msg_name(char type) {
    switch (type) {
        case 'A': return "Add Order";
        case 'F': return "Add Order with MPID";
        case 'E': return "Order Executed";
        case 'C': return "Order Executed with Price";
        case 'X': return "Order Cancel";
        case 'D': return "Order Delete";
        case 'U': return "Order Replace";
        case 'P': return "Trade (non-cross)";
        case 'S': return "System Event";
        case 'H': return "Stock Trading Action";
        default:  return "Unknown / not-yet-handled";
    }
}

// recieves market data
MarketDataPtr recv_market_data() {
    // This recieved the raw binary market data from the exchange via multicast UDP
    MarketDataPtr buf = std::make_shared<std::array<char, 1024>>();

    ssize_t bytes = recvfrom(sock_fd, buf->data(), buf->size(), 0, nullptr, nullptr);

    char type = (*buf)[0];
    std::cout << "Received " << bytes << " bytes"
              << " | type '" << type << "'"
              << " (0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << (static_cast<unsigned int>(type) & 0xFF) << std::dec << ")"
              << " -> " << itch_msg_name(type) << "\n";

    return buf;
}

void close_socket() {
    close(sock_fd);
}