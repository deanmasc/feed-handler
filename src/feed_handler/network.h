// Declarations for all feed handler side networking functions/variables

#ifndef FEED_HANDLER_HEADER
#define FEED_HANDLER_HEADER

constexpr char* MULTICAST_IP_ADDR {"239.0.0.1"};
constexpr int MULTICAST_PORT {30000};

void setup_socket();
void recv_market_data(char data[]);
void close_socket();


#endif