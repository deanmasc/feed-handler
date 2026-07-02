#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstdint>
#include "data_models.h"

struct OrderEntry {
    Side side;
    uint32_t price;
    uint32_t volume;
};
#endif