#include <unordered_map>
#include <array>
#include <tuple>
#include "order_book.h"

using Orders = std::unordered_map<uint64_t, OrderEntry>;
using PriceLevels = std::unordered_map<uint32_t, std::unordered_map<Side, std::array<uint32_t, 2>>>;

class OrderBook {
private:
    Orders orders;
    PriceLevels price_levels;
    static constexpr size_t VOLUME_IDX {0};
    static constexpr size_t MSG_COUNT_IDX {1};

public:
    OrderBook() {}

    bool set_order_volume(uint64_t order_ref, uint32_t volume_delta) {
        if (!orders.count(order_ref)) {
            return false;
        }

        orders[order_ref].volume += volume_delta;
        return true;
    }

    bool set_order_price(uint64_t order_ref, uint32_t new_price) {
        if (!orders.count(order_ref)) {
            return false;
        }

        orders[order_ref].price = new_price;
        return true;
    }

    bool adjust_price_level_volume(uint64_t order_ref, uint32_t volume_delta) {
        if (!orders.count(order_ref)) {
            return false;
        }

        price_levels[orders[order_ref].price][orders[order_ref].side][VOLUME_IDX] += volume_delta;
        return true;
    }

    bool adjust_price_level_msg_count(uint64_t order_ref, int increment) {
        if (!orders.count(order_ref)) {
            return false;
        }

        price_levels[orders[order_ref].price][orders[order_ref].side][MSG_COUNT_IDX] += increment;
        return true;
    }

    bool add_order(uint64_t order_ref, Side side, uint32_t price, uint32_t volume) {
        if (orders.count(order_ref)) {
            return false;
        }

        orders[order_ref] = OrderEntry {side, price, volume};
        if (!adjust_price_level_volume(order_ref, volume)) {
            return false;
        } 
        if (!adjust_price_level_msg_count(order_ref, 1)) {
            return false;
        }
        return true;
    }

    bool delete_order(uint64_t order_ref) {
        if (!orders.count(order_ref)) {
            return false;
        }

        if (!adjust_price_level_volume(order_ref, -orders[order_ref].volume)) {
            return false;
        } 
        if (!adjust_price_level_msg_count(order_ref, -1)) {
            return false;
        }

        orders.erase(order_ref);
        return true;
    }

    bool apply(const OrderAdd& order) {
        if (!add_order(order.order_ref, order.side, order.price, order.volume)) {
            return false;
        }
        return true;
    }

    bool apply(const OrderAddMPID& order) {
       if (!add_order(order.order_ref, order.side, order.price, order.volume)) {
            return false;
        }
        return true;
    }

    bool apply(const OrderExecuted& order) {
        return true;
    }

    bool apply(const OrderExecutedWithPrice& order) {
        return true;
    }

    bool apply(const OrderCancel& order) {

        return true;
    }

    bool apply(const OrderDelete& order) {
        if (!delete_order(order.order_ref)) {
            return false;
        }
        return true;
    }

    bool apply(const OrderReplace& order) {
        return true;
    }
};