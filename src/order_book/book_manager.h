#ifndef BOOK_HANDLER_H
#define BOOK_HANDLER_H

#include <unordered_map>
#include <cstdint>
#include <optional>
#include "order_book.h"

class BookManager {
private:
    std::unordered_map<uint16_t, OrderBook> order_books;
public:
    BookManager() {};

    template<typename T>
    bool apply(T& order) {
        return order_books[order.msg_header.stock_locate].apply(order);
    }
};

#endif