// Declarations for all exchange side networking functions/variables

#ifndef EXCHANGE_HEADER
#define EXCHANGE_HEADER

#include <map>
#include <mutex>

constexpr const char* MULTICAST_IP_ADDR {"239.0.0.1"};
constexpr const int MULTICAST_PORT {30000};
constexpr const char* RETRANSMISSION_IP_ADDR {"127.0.0.1"};
constexpr const int RETRANSMISSION_PORT {30001};
constexpr const size_t RETRANSMISSION_BUFFER_MAX_SIZE {10}; // Amount of packets buffered

struct BufferedPacket {
    std::array<char, 1024> data;
    uint16_t bytes;
    uint16_t messages_lost;
};

void setup_socket();
void send_market_data(const char* data, size_t len);
void recv_retransmission_reqs(const std::map<uint64_t, BufferedPacket>& packet_buffer, std::mutex& buf_mtx);
void close_socket();

#endif