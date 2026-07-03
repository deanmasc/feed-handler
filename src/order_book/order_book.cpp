#include "order_book.h"

OrderBook::OrderBook() {}

bool OrderBook::set_order_volume(uint64_t order_ref, uint32_t new_volume) {
    if (!orders.count(order_ref)) {
        return false;
    }

    orders[order_ref].volume = new_volume;
    return true;
}

bool OrderBook::set_order_price(uint64_t order_ref, uint32_t new_price) {
    if (!orders.count(order_ref)) {
        return false;
    }

    orders[order_ref].price = new_price;
    return true;
}

// Return the value if the ref exists, otherwise nullopt (caller handles the missing case).
std::optional<uint32_t> OrderBook::get_order_volume(uint64_t order_ref) const {
    if (!orders.count(order_ref)) {
        return std::nullopt;
    }
    return orders.at(order_ref).volume;
}

std::optional<uint32_t> OrderBook::get_order_price(uint64_t order_ref) const {
    if (!orders.count(order_ref)) {
        return std::nullopt;
    }
    return orders.at(order_ref).price;
}

std::optional<Side> OrderBook::get_order_side(uint64_t order_ref) const {
    if (!orders.count(order_ref)) {
        return std::nullopt;
    }
    return orders.at(order_ref).side;
}

bool OrderBook::adjust_price_level_volume(uint64_t order_ref, int64_t volume_delta) {
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

bool OrderBook::adjust_price_level_msg_count(uint64_t order_ref, int increment) {
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

bool OrderBook::add_order(uint64_t order_ref, Side side, uint32_t price, uint32_t volume) {
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

bool OrderBook::delete_order(uint64_t order_ref) {
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

bool OrderBook::adjust_order(uint64_t order_ref, int64_t volume_delta) {
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



bool OrderBook::apply(const OrderAdd& order) {
    return add_order(order.order_ref, order.side, order.price, order.volume);
}

bool OrderBook::apply(const OrderAddMPID& order) {
   return add_order(order.order_ref, order.side, order.price, order.volume);
}

bool OrderBook::apply(const OrderExecuted& order) {
    return adjust_order(order.order_ref, -static_cast<int64_t>(order.executed_shares));
}

bool OrderBook::apply(const OrderExecutedWithPrice& order) {
    return adjust_order(order.order_ref, -static_cast<int64_t>(order.executed_shares));
}

bool OrderBook::apply(const OrderCancel& order) {
    return adjust_order(order.order_ref, -static_cast<int64_t>(order.cancelled_shares));
}

bool OrderBook::apply(const OrderDelete& order) {
    return delete_order(order.order_ref);
}

bool OrderBook::apply(const OrderReplace& order) {
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
