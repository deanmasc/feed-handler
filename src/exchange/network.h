// Declarations for all exchange side networking functions/variables

#ifndef EXCHANGE_HEADER
#define EXCHANGE_HEADER

constexpr std::string_view MULTICAST_IP_ADDR {"239.0.0.1"};
constexpr int MULTICAST_PORT {30000};

void setup_socket();
void send_market_data(char data[]);
void close_socket();

#endif