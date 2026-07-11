// Declarations for all exchange side networking functions/variables

#ifndef EXCHANGE_HEADER
#define EXCHANGE_HEADER

#include <map>
#include <mutex>

constexpr const char* MULTICAST_IP_ADDR {"239.0.0.1"};
constexpr const int MULTICAST_PORT {30000};
constexpr const size_t RETRANSMISSION_BUFFER_MAX_SIZE {10}; // Amount of packets buffered

struct BufferedPacket {
    std::array<char, 1024> data;
    uint16_t bytes;
};

void setup_socket();
void send_market_data(char* data, size_t len);
void recv_retransmission_reqs(std::map<uint64_t, BufferedPacket>& packet_buffer, std::mutex& buf_mtx);
void close_socket();

#endif