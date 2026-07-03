#ifndef FEED_HANDLER_HEADER
#define FEED_HANDLER_HEADER

#include <unordered_map>
#include <cstdint>
#include <optional>
#include "order_book.h"

class BookManager {
private:
    std::unordered_map<uint16_t, OrderBook> order_books;
public:
    BookManager() {};

    template<typename Order>
    bool apply(Order& order) {
        return order_books[order.msg_header.stock_locate].apply(order);
    }
};

#endif