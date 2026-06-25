#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include "exchange_network.h"

int sock_fd;
sockaddr_in dest {};


void setup_socket() {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // destination: the multicast group
    dest.sin_family = AF_INET;
    dest.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_IP_ADDR, &dest.sin_addr);
    int loop = 1;
    setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    // send multicast out via the loopback interface (same-machine delivery)
    in_addr iface{};
    inet_pton(AF_INET, "127.0.0.1", &iface.s_addr);
    setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface));
}


// dummy function for now
void send_market_data(char* data, size_t len) {
    // This function will be called with MoldUDP64 wrapped market data to send to the multicast UDP
    ssize_t sent = sendto(sock_fd, data, len, 0, (sockaddr*)&dest, sizeof(dest));
    if (sent < 0) {
        std::cout << "sendto FAILED: " << std::strerror(errno) << std::endl;
    } else {
        std::cout << "Sent " << sent << " bytes" << std::endl;
    }
}


void close_socket() {
    close(sock_fd);
}