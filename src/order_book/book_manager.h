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

    std::optional<OrderBook*> get_order_book(uint16_t stock_locate) {
        if (!order_books.count(stock_locate)) {
            return std::nullopt;
        }

        return &order_books.at(stock_locate);
    }

    template<typename T>
    bool apply(T& order) {
        return order_books[order.msg_header.stock_locate].apply(order);
    }
};

#endif