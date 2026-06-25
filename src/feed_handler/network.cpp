#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <array>
#include <network.h>

int sock_fd;
sockaddr_in dest {};


void setup_socket() {

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    int reuse = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock_fd, (sockaddr*)&addr, sizeof(addr));

    ip_mreq group{};
    inet_pton(AF_INET, "239.1.2.3", &group.imr_multiaddr);
    group.imr_interface.s_addr = INADDR_ANY;
    setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));

    close(sock_fd);
}


// dummy function for now
std::array<char, 1024> recv_market_data(char data[]) {
    // This function will be called with MoldUDP64 wrapped market data to send to the multicast UDP
    char buf[1024];
    recvfrom(sock_fd, buf, sizeof(buf), 0, nullptr, nullptr);
}

void close_socket() {
    close(sock_fd);
}