#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstdint>
#include <unordered_map>
#include <array>
#include <optional>
#include "data_models.h"

struct OrderEntry {
    Side side;
    uint32_t price;
    uint32_t volume;
};

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

    bool set_order_volume(uint64_t order_ref, uint32_t new_volume) {
        if (!orders.count(order_ref)) {
            return false;
        }

        orders[order_ref].volume = new_volume;
        return true;
    }

    bool set_order_price(uint64_t order_ref, uint32_t new_price) {
        if (!orders.count(order_ref)) {
            return false;
        }

        orders[order_ref].price = new_price;
        return true;
    }

    // Return the value if the ref exists, otherwise nullopt (caller handles the missing case).
    std::optional<uint32_t> get_order_volume(uint64_t order_ref) const {
        if (!orders.count(order_ref)) {
            return std::nullopt;
        }
        return orders.at(order_ref).volume;
    }

    std::optional<uint32_t> get_order_price(uint64_t order_ref) const {
        if (!orders.count(order_ref)) {
            return std::nullopt;
        }
        return orders.at(order_ref).price;
    }

    std::optional<Side> get_order_side(uint64_t order_ref) const {
        if (!orders.count(order_ref)) {
            return std::nullopt;
        }
        return orders.at(order_ref).side;
    }

    bool adjust_price_level_volume(uint64_t order_ref, int64_t volume_delta) {
        if (!orders.count(order_ref)) {
            return false;
        }

        if (price_levels[orders[order_ref].price][orders[order_ref].side][VOLUME_IDX] + volume_delta <= 0) {
            price_levels[orders[order_ref].price][orders[order_ref].side][VOLUME_IDX] = 0;
        } else {
            price_levels[orders[order_ref].price][orders[order_ref].side][VOLUME_IDX] += volume_delta;
        }

        return true;
    }

    bool adjust_price_level_msg_count(uint64_t order_ref, int increment) {
        if (!orders.count(order_ref)) {
            return false;
        }

        price_levels[orders[order_ref].price][orders[order_ref].side][MSG_COUNT_IDX] += increment;

        // Make sure to clean up price levels
        if (price_levels[orders[order_ref].price][orders[order_ref].side][MSG_COUNT_IDX] == 0) {
            price_levels[orders[order_ref].price].erase(orders[order_ref].side);
            if (price_levels[orders[order_ref].price].empty()) {
                price_levels.erase(orders[order_ref].price);
            }

        }
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

        if (!adjust_price_level_volume(order_ref, -static_cast<int64_t>(orders[order_ref].volume))) {
            return false;
        }
        if (!adjust_price_level_msg_count(order_ref, -1)) {
            return false;
        }

        orders.erase(order_ref);
        return true;
    }

    bool adjust_order(uint64_t order_ref, int64_t volume_delta) {
        if (!orders.count(order_ref)) {
            return false;
        }

        if (orders[order_ref].volume + volume_delta <= 0) {
            // this is the same as a delete
            return delete_order(order_ref);
        }

        if (!adjust_price_level_volume(order_ref, volume_delta)) {
            return false;
        }

        if (!set_order_volume(order_ref, static_cast<uint32_t>(orders[order_ref].volume + volume_delta))) {
            return false;
        }

        return true;
    }



    bool apply(const OrderAdd& order) {
        return add_order(order.order_ref, order.side, order.price, order.volume);
    }

    bool apply(const OrderAddMPID& order) {
       return add_order(order.order_ref, order.side, order.price, order.volume);
    }

    bool apply(const OrderExecuted& order) {
        return adjust_order(order.order_ref, -static_cast<int64_t>(order.executed_shares));
    }

    bool apply(const OrderExecutedWithPrice& order) {
        return adjust_order(order.order_ref, -static_cast<int64_t>(order.executed_shares));
    }

    bool apply(const OrderCancel& order) {
        return adjust_order(order.order_ref, -static_cast<int64_t>(order.cancelled_shares));
    }

    bool apply(const OrderDelete& order) {
        return delete_order(order.order_ref);
    }

    bool apply(const OrderReplace& order) {
        std::optional<Side> side {get_order_side(order.original_order_ref)};
        if (!side) {
            // unknown original ref - can't replace, return false
            return false;
        }

        // Delete the old order
        if (!delete_order(order.original_order_ref)) {
            return false;
        }

        // Add the new order
        return add_order(order.new_order_ref, *side, order.price, order.volume);

    }
};

#endif
