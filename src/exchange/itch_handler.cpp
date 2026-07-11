#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <set>
#include <thread>
#include "exchange_network.h"
#define htobe64(x) __builtin_bswap64(x)

constexpr const char* ITCH_FILE = "/Users/deanmascitti/Desktop/dean-dev/feed-handler/src/data/test.NASDAQ_ITCH50";
const std::set<char> VALID_TYPES {'A', 'F', 'E', 'C', 'X', 'D', 'U', 'P', 'S', 'H'};

void wrap_MoldUDP64_header(char* buf, uint64_t seq_num, uint16_t message_count) {
    memcpy(&buf[0], "SESSION  1", 10);

    uint64_t seq_num_big_endian = htobe64(seq_num);
    memcpy(&buf[10], &seq_num_big_endian, 8);

    message_count = htons(message_count);
    memcpy(&buf[18], &message_count, 2);

}

void read_and_send_itch_data(std::map<uint64_t, BufferedPacket>& packet_buffer, std::mutex& buf_mtx) {
    uint64_t seq_num {1};
    uint16_t message_count {1};
    std::ifstream file(ITCH_FILE, std::ios::binary);

    if (!file) {
        std::cout << "Failed to open ITCH file: " << ITCH_FILE << std::endl;
        return;
    }

    std::array<char, 1024> buf;
    while (true) {
        // 1) read the 2-byte length prefix
        unsigned char len_bytes[2];
        file.read(reinterpret_cast<char*>(len_bytes), 2);
        memcpy(&buf[20], reinterpret_cast<char*>(len_bytes), 2);

        if (file.gcount() < 2) break;  // EOF or partial — done

        uint16_t packet_len = (len_bytes[0] << 8) | len_bytes[1];  // big-endian -> host
        if (packet_len == 0 || packet_len > sizeof(buf)) break;       // sanity guard

        // 2) read exactly that many bytes — one complete ITCH message
        file.read(buf.data() + 22, packet_len);
        if (file.gcount() < packet_len) break;  // truncated tail

        // 3) hand the raw message to the sender (no decoding yet)
        // Add checking to ensure we are sending only relevant types
        char type = buf[22];
        if (VALID_TYPES.count(type)) {
            wrap_MoldUDP64_header(buf.data(), seq_num, message_count);
            {   
                std::lock_guard<std::mutex> lock(buf_mtx);
                packet_buffer[seq_num] = BufferedPacket {buf, static_cast<uint16_t>(packet_len + 22)};
                if (packet_buffer.size() > RETRANSMISSION_BUFFER_MAX_SIZE) {
                    // removing the oldest packet if we have exceed capacity
                    packet_buffer.erase(packet_buffer.begin()->first);
                }
                send_market_data(buf.data(), packet_len + 22);
                ++seq_num;
            }
        }
    }

    std::cout << "Reached end of ITCH file" << std::endl;
}

void handle_itch_processing() {
    std::map<uint64_t, BufferedPacket> packet_buffer;
    std::mutex buf_mtx;

    // 2 threads for sending itch data, and recieving retransmission requests
    std::thread  send_data_thread(read_and_send_itch_data, std::ref(packet_buffer), std::ref(buf_mtx));
    std::thread  recv_data_thread(recv_retransmission_reqs, std::ref(packet_buffer), std::ref(buf_mtx));

    send_data_thread.join();
    recv_data_thread.join();
}