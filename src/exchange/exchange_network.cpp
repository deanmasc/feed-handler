#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include "exchange_network.h"

int sock_fd;
int sock_fd_recv;

sockaddr_in addr {};
sockaddr_in dest {};


void setup_socket() {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    sock_fd_recv = socket(AF_INET, SOCK_DGRAM, 0);

    // destination: the multicast group
    dest.sin_family = AF_INET;
    dest.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_IP_ADDR, &dest.sin_addr);

    // address: the address of the exchange process
    addr.sin_family = AF_INET;
    addr.sin_port = htons(RETRANSMISSION_PORT);
    inet_pton(AF_INET, RETRANSMISSION_IP_ADDR, &addr.sin_addr);
    bind(sock_fd_recv, (sockaddr*)&addr, sizeof(addr));

    int loop = 1;
    setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    // send multicast out via the loopback interface (same-machine delivery)
    in_addr iface{};
    inet_pton(AF_INET, "127.0.0.1", &iface.s_addr);
    setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface));
}


// dummy function for now
void send_market_data(const char* data, size_t len) {
    // This function will be called with MoldUDP64 wrapped market data to send to the multicast UDP
    ssize_t sent = sendto(sock_fd, data, len, 0, (sockaddr*)&dest, sizeof(dest));
    if (sent < 0) {
        std::cout << "sendto FAILED: " << std::strerror(errno) << std::endl;
    } else {
        std::cout << "Sent " << sent << " bytes" << std::endl;
    }
}

void recv_retransmission_reqs(const std::map<uint64_t, BufferedPacket>& packet_buffer, std::mutex& buf_mtx) {
    std::array<char, 1024> buf;

    while (true) {
        ssize_t bytes = recvfrom(sock_fd_recv, buf.data(), buf.size(), 0, nullptr, nullptr);

        if (bytes < 10) {
            // Not enough bytes recieved for the expected information
            continue;
        }

        size_t offset {};
        uint64_t first_seq_num;
        memcpy(&first_seq_num, &buf[offset], 8);
        offset += 8;

        uint16_t messages_lost;
        memcpy(&messages_lost, &buf[offset], 2);

        std::cout << "Exchange is aware of " << messages_lost 
                  << " messages lost, from sequence number "
                  << first_seq_num << std::endl;

        {
            std::lock_guard<std::mutex> lock(buf_mtx);
            if (packet_buffer.empty() || !packet_buffer.count(first_seq_num)) {
                continue;
            }

            BufferedPacket lost_packet {packet_buffer.at(first_seq_num)};
            send_market_data(&lost_packet.data[0], lost_packet.bytes);

            std::cout << "Exchange has sent back " << messages_lost 
                  << " messages that were lost, from sequence number "
                  << first_seq_num << std::endl;
        }
    }
}


void close_socket() {
    close(sock_fd);
    close(sock_fd_recv);
}