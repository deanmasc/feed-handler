#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>   // ntohs
#include "exchange_network.h"

constexpr const char* ITCH_FILE = "/Users/deanmascitti/Desktop/dean-dev/feed-handler/src/data/07302019.NASDAQ_ITCH50";

int main() {
    std::cout << "Exchange process started" << std::endl;

    // First thing we want to do is create the multicast UDP socket
    setup_socket();

    std::ifstream file(ITCH_FILE, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open ITCH file: " << ITCH_FILE << std::endl;
        return 1;
    }

    // The raw ITCH file frames each message with a 2-byte big-endian length prefix:
    //   [2-byte length][message bytes][2-byte length][message bytes]...
    char buf[1024];
    while (true) {
        // 1) read the 2-byte length prefix
        unsigned char len_bytes[2];
        file.read(reinterpret_cast<char*>(len_bytes), 2);
        if (file.gcount() < 2) break;  // EOF or partial — done

        uint16_t msg_len = (len_bytes[0] << 8) | len_bytes[1];  // big-endian -> host
        if (msg_len == 0 || msg_len > sizeof(buf)) break;       // sanity guard

        // 2) read exactly that many bytes — one complete ITCH message
        file.read(buf, msg_len);
        if (file.gcount() < msg_len) break;  // truncated tail

        // 3) hand the raw message to the sender (no decoding yet)
        send_market_data(buf, msg_len);

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    std::cout << "Reached end of ITCH file" << std::endl;
    close_socket();
    return 0;
}