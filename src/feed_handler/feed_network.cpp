#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <iomanip>
#include "feed_network.h"
#include "../parser/parser.h"

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

void request_retransmission(uint64_t first_seq_num, uint16_t messages_lost) {
    std::cout << "Retransmission request for " << messages_lost 
              << " messages lost, starting from sequence number " << first_seq_num
              << std::endl;

    return;
}

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
void recv_market_data(BookManager& book_manager, 
                      uint64_t& expected_seq_num, 
                      std::set<uint64_t>& packets_lost, 
                      std::map<uint64_t, PacketData>& packet_buffer) {
    // This recieved the raw binary market data from the exchange via multicast UDP
    std::array<char, 1024> buf;

    ssize_t bytes = recvfrom(sock_fd, buf.data(), buf.size(), 0, nullptr, nullptr);

    char session[10];
    memcpy(session, buf.data(), 10);

    uint64_t packet_seq_num;
    memcpy(&packet_seq_num, buf.data() + 10, 8);
    packet_seq_num = __builtin_bswap64(packet_seq_num);

    uint16_t message_count;
    memcpy(&message_count, buf.data() + 18, 2);
    message_count = ntohs(message_count);

    if (packet_seq_num == expected_seq_num) { // Expected seq_num (can be buffered or sent)
        if (!packets_lost.empty()) {
            packet_buffer[packet_seq_num] = PacketData{packet_seq_num, message_count, buf};
        } else {
            expected_seq_num += message_count;
            char type = buf[22];
            std::cout << "Received " << bytes << " bytes "
                    << "Session: " << std::string(session, 10) << " "
                    << "Sequence Number " << packet_seq_num << " "
                    << "Message_count " << message_count << " "
                    << " | type '" << type << "'"
                    << " (0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                    << (static_cast<unsigned int>(type) & 0xFF) << std::dec << ")"
                    << " -> " << itch_msg_name(type) << "\n";

            process_message(buf.data() + 22, bytes - 22, book_manager);
        }

    } else if (packets_lost.contains(packet_seq_num)) { // Recieved lost packet (can be buffered or sent)
        std::cout << "RECIEVED LOST PACKET! Starting Seq num: " << packet_seq_num 
                  << ", message count: " << message_count << "."
                  << std::endl;
        
        for (size_t i{packet_seq_num}; i < packet_seq_num + message_count; i++) {
            packets_lost.erase(i);
        }

        if (*packets_lost.begin() == packet_seq_num) {
            process_message(buf.data() + 22, bytes - 22, book_manager);
        } else {
            packet_buffer[packet_seq_num] = PacketData{packet_seq_num, message_count, buf};
        }

    } else { // Identified lost packet (incoming packet will be buffered)
        std::cout << "PACKET LOST! Expected Seq num " << expected_seq_num 
                  << ", recieved seq num " << packet_seq_num << ". "
                  << "Sending retransmission request to exchange."
                  << std::endl;

        // Insert all lost seq_nums
        for (size_t i {expected_seq_num}; i < packet_seq_num; i++) {
            packets_lost.insert(i);
        }
        request_retransmission(expected_seq_num, packet_seq_num - expected_seq_num); // Second argument is the amount of messages lost

        // Now we buffer/store the packet we did recieve
        packet_buffer[packet_seq_num] = PacketData{packet_seq_num, message_count, buf};
    }

}

void handle_recv_market_data(BookManager& book_manager) {
    uint64_t expected_seq_num {1};
    std::set<uint64_t> packets_lost;
    std::map<uint64_t, PacketData> packet_buffer;

    while (true) {
        recv_market_data(book_manager, expected_seq_num, packets_lost, packet_buffer);
        if (packets_lost.empty()) {
            // Call some function to unload packet_buffer
        }
    }
}

void close_socket() {
    close(sock_fd);
}