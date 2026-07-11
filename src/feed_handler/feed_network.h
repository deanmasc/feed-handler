// Declarations for all feed handler side networking functions/variables

#ifndef FEED_HANDLER_HEADER
#define FEED_HANDLER_HEADER

#include <set>
#include <map>
#include <array>
#include <unistd.h>
#include <optional>
#include "../order_book/book_manager.h"

constexpr const char* MULTICAST_IP_ADDR {"239.0.0.1"};
constexpr const int MULTICAST_PORT {30000};
constexpr const char* RETRANSMISSION_IP_ADDR {"127.0.0.1"};
constexpr const int RETRANSMISSION_PORT {30001};

struct PacketData {
    uint64_t start_seq_num;
    uint16_t message_count;
    uint16_t bytes;
    std::array<char, 1024> data;
};

struct PacketDataToSend {
    char* buf_data;
    uint16_t msg_len;
};

void setup_socket();
void handle_recv_market_data(BookManager& book_manager);
void close_socket();


#endif