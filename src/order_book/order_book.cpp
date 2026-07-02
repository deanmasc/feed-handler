#include <unordered_map>
#include <array>
#include <tuple>
#include "order_book.h"

using Orders = std::unordered_map<uint64_t, OrderEntry>;
using PriceLevels = std::unordered_map<uint32_t, std::unordered_map<Side, std::array<uint32_t, 2>>;

class OrderBook {
private:
    Orders orders;
    PriceLevels price_levels;
public:
    OrderBook() {}
}