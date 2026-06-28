#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <set>
#include "exchange_network.h"
#define htobe64(x) __builtin_bswap64(x)

constexpr const char* ITCH_FILE = "/Users/deanmascitti/Desktop/dean-dev/feed-handler/src/data/test.NASDAQ_ITCH50";
const std::set<char> VALID_TYPES {'A', 'F', 'E', 'C', 'X', 'D', 'U', 'P', 'S', 'H'};

void wrap_MoldUDP64_header(char* buf, uint64_t seq_num) {
    uint16_t message_count {1};

    memcpy(&buf[0], "SESSION  1", 10);

    uint64_t seq_num_big_endian = htobe64(seq_num);
    memcpy(&buf[10], &seq_num_big_endian, 8);

    message_count = htons(message_count);
    memcpy(&buf[18], &message_count, 2);

}

void read_and_send_itch_data() {
    uint64_t seq_num {1};
    std::ifstream file(ITCH_FILE, std::ios::binary);

    if (!file) {
        std::cout << "Failed to open ITCH file: " << ITCH_FILE << std::endl;
        return;
    }

    char buf[1024];
    while (true) {
        // 1) read the 2-byte length prefix
        unsigned char len_bytes[2];
        file.read(reinterpret_cast<char*>(len_bytes), 2);
        memcpy(&buf[20], reinterpret_cast<char*>(len_bytes), 2);

        if (file.gcount() < 2) break;  // EOF or partial — done

        uint16_t msg_len = (len_bytes[0] << 8) | len_bytes[1];  // big-endian -> host
        if (msg_len == 0 || msg_len > sizeof(buf)) break;       // sanity guard

        // 2) read exactly that many bytes — one complete ITCH message
        file.read(buf + 22, msg_len);
        if (file.gcount() < msg_len) break;  // truncated tail

        // 3) hand the raw message to the sender (no decoding yet)
        // Add checking to ensure we are sending only relevant types
        char type = buf[22];
        if (VALID_TYPES.count(type)) {
            wrap_MoldUDP64_header(buf, seq_num);
            send_market_data(buf, msg_len + 22);
            ++seq_num;
            std::this_thread::sleep_for(std::chrono::microseconds(100000));
        }

    }

    std::cout << "Reached end of ITCH file" << std::endl;
}