#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstdint>
#include <unordered_map>
#include <array>
#include <optional>
#include <vector>
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

    std::vector<uint32_t> get_all_prices(Side side) const;

    bool adjust_price_level_volume(uint64_t order_ref, int64_t volume_delta);
    bool adjust_price_level_msg_count(uint64_t order_ref, int increment);

    bool add_order(uint64_t order_ref, Side side, uint32_t price, uint32_t volume);
    bool delete_order(uint64_t order_ref);
    bool adjust_order(uint64_t order_ref, int64_t volume_delta);

public:
    OrderBook();

    bool set_order_volume(uint64_t order_ref, uint32_t new_volume);
    bool set_order_price(uint64_t order_ref, uint32_t new_price);

    // Return the value if the ref exists, otherwise nullopt (caller handles the missing case).
    std::optional<uint32_t> get_order_volume(uint64_t order_ref) const;
    std::optional<uint32_t> get_order_price(uint64_t order_ref) const;
    std::optional<Side> get_order_side(uint64_t order_ref) const;
    std::optional<uint32_t> get_price_level_volume(uint32_t price, Side side) const;
    std::optional<uint32_t> get_price_level_msg_count(uint32_t price, Side side) const;
    std::vector<uint32_t> get_all_bids() const;
    std::vector<uint32_t> get_all_asks() const;

    bool apply(const OrderAdd& order);
    bool apply(const OrderAddMPID& order);
    bool apply(const OrderExecuted& order);
    bool apply(const OrderExecutedWithPrice& order);
    bool apply(const OrderCancel& order);
    bool apply(const OrderDelete& order);
    bool apply(const OrderReplace& order);
};

#endif
