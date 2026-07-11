#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

static void request_retransmission(uint64_t first_seq_num, uint16_t bytes) {
    std::cout << "Retransmission request for " << bytes 
              << " bytes lost, starting from sequence number " << first_seq_num
              << std::endl;

    char data[10];
    memcpy(&data[0], &first_seq_num, 8);
    memcpy(&data[8], &bytes, 2);

    ssize_t sent = sendto(sock_fd, data, 10, 0, (sockaddr*)&addr, sizeof(addr));

    if (sent < 0) {
        std::cout << "sendto FAILED for retransmission request: " << std::strerror(errno) << std::endl;
    } else {
        std::cout << "Retransmission request sent " << sent << " bytes" << std::endl;
    }

    return;
}

static void empty_buffer(std::map<uint64_t, PacketData>& packet_buffer, BookManager& book_manager) {
    // unload packet_buffer
    for (auto& [start_seq_num, packet_data] : packet_buffer) {
        // keys come out in ascending order automatically
        // this works for now as we know 1 message per packet but this will
        // be subject to change
        process_message(packet_data.data.data() + 22, packet_data.bytes - 22, book_manager);
    }
    packet_buffer.clear();
}

static void erase_lost_messages(std::set<uint64_t>& messages_lost, uint64_t start_seq_num, uint16_t message_count) {
    for (size_t i{}; i < message_count; i++) {
        messages_lost.erase(start_seq_num + i);
    }
}

static void insert_lost_messages(std::set<uint64_t>& messages_lost, uint64_t expected_seq_num, uint64_t packet_seq_num) {
    for (size_t i {expected_seq_num}; i < packet_seq_num; i++) {
        messages_lost.insert(i);
    }
}

// recieves market data
std::optional<PacketDataToSend> recv_market_data(std::array<char, 1024>& buf, 
                      uint64_t& expected_seq_num, 
                      std::set<uint64_t>& messages_lost, 
                      std::map<uint64_t, PacketData>& packet_buffer) {

    ssize_t bytes = recvfrom(sock_fd, buf.data(), buf.size(), 0, nullptr, nullptr);

    if (bytes < 0) {
        // Some error recieving
        return std::nullopt;
    }

    uint16_t u_bytes = static_cast<uint16_t>(bytes);

    char session[10];
    memcpy(session, buf.data(), 10);

    uint64_t packet_seq_num;
    memcpy(&packet_seq_num, buf.data() + 10, 8);
    packet_seq_num = __builtin_bswap64(packet_seq_num);

    uint16_t message_count;
    memcpy(&message_count, buf.data() + 18, 2);
    message_count = ntohs(message_count);

    char type = buf[22];
    std::cout << "Received " << u_bytes << " bytes "
            << "Session: " << std::string(session, 10) << " "
            << "Sequence Number " << packet_seq_num << " "
            << "Message_count " << message_count << " "
            << " | type '" << type << "'"
            << " (0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << (static_cast<unsigned int>(type) & 0xFF) << std::dec << ")"
            << " -> " << itch_msg_name(type) << "\n";

    if (packet_seq_num == expected_seq_num) { // Expected seq_num (can be buffered or sent)
        expected_seq_num += message_count;
        if (!messages_lost.empty()) {
            packet_buffer[packet_seq_num] = PacketData{packet_seq_num, message_count, u_bytes, buf};
        } else {
            return PacketDataToSend {buf.data() + 22, static_cast<uint16_t>(u_bytes - 22)};
        }

    } else if (messages_lost.contains(packet_seq_num)) { // Recieved lost packet (can be buffered or sent)
        std::cout << "RECIEVED LOST PACKET! Starting Seq num: " << packet_seq_num 
                  << ", message count: " << message_count << "."
                  << std::endl;

        uint64_t min_lost_packet {*messages_lost.begin()};
        erase_lost_messages(messages_lost, packet_seq_num, message_count);

        if (min_lost_packet == packet_seq_num) { // If the lost packet is valid to be sent next
            return PacketDataToSend {buf.data() + 22, static_cast<uint16_t>(u_bytes - 22)};
        } else { // If not then we buffer
            packet_buffer[packet_seq_num] = PacketData{packet_seq_num, message_count, u_bytes, buf};
        }

    }  else if (packet_seq_num > expected_seq_num) { // Identified lost packet (because if packet seq num was smaller then we recieved a duplicate)
        std::cout << "PACKET LOST! Expected Seq num " << expected_seq_num 
                  << ", recieved seq num " << packet_seq_num << ". "
                  << "Sending retransmission request to exchange."
                  << std::endl;

        // Insert all lost seq_nums
        insert_lost_messages(messages_lost, expected_seq_num, packet_seq_num);
        request_retransmission(expected_seq_num, u_bytes);

        expected_seq_num = packet_seq_num + message_count;
        // Now we buffer/store the packet we did recieve
        packet_buffer[packet_seq_num] = PacketData{packet_seq_num, message_count, u_bytes, buf};
    }

    return std::nullopt;

}

void handle_recv_market_data(BookManager& book_manager) {
    uint64_t expected_seq_num {1};
    std::set<uint64_t> messages_lost;
    std::map<uint64_t, PacketData> packet_buffer;
    std::optional<PacketDataToSend> packet_data_to_send;
    std::array<char, 1024> buf;

    while (true) {
        packet_data_to_send = recv_market_data(buf, expected_seq_num, messages_lost, packet_buffer);
        if (packet_data_to_send) {
            process_message(packet_data_to_send->buf_data, packet_data_to_send->msg_len, book_manager);
            // If no packets lost and buffer has entries, then we empty the buffer
            if (messages_lost.empty() && !packet_buffer.empty()) {
                empty_buffer(packet_buffer, book_manager);
            }
        }
    }
}

void close_socket() {
    close(sock_fd);
} 