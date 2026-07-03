// Declarations for all feed handler side networking functions/variables

#ifndef FEED_HANDLER_HEADER
#define FEED_HANDLER_HEADER

#include "../order_book/book_manager.h"

using MarketDataPtr = std::shared_ptr<std::array<char, 1024>>;
constexpr const char* MULTICAST_IP_ADDR {"239.0.0.1"};
constexpr const int MULTICAST_PORT {30000};

void setup_socket();
void recv_market_data(BookManager& book_manager);
void close_socket();


#endif