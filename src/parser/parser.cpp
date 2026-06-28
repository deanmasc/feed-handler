#include <memory>
#include <iostream>
#include <string>
#include <cstring>      // memcpy
#include <cstdint>      // uintN_t
#include <arpa/inet.h>  // ntohs, ntohl

void process_add_order(char* msg, uint16_t len) {
    if (len < 36) {
        std::cout << "Add Order has incomplete data " << len << "/36 bytes" << std::endl;
        return;
    }
    // Defining all fields
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0}; // only uses 48 bytes
    uint64_t order_ref;
    char side; // 'B' or 'S'
    uint32_t volume;
    char symbol[8];
    uint32_t price;
    int offset {1};

    // Assigning all fields

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate); // big to little endian
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num); // big to little endian
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2 , msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    memcpy(&side, msg + offset, 1);
    ++offset;

    memcpy(&volume, msg + offset, 4);
    volume = ntohl(volume);
    offset += 4;

    memcpy(symbol, msg + offset, 8);
    offset += 8;

    memcpy(&price, msg + offset, 4);
    price = ntohl(price);
    offset += 4;

    // Now we create the Add order class or struct

    std::cout << "\n"
              << "[Add Order]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " side=" << side
              << " volume=" << volume
              << " symbol=" << std::string(symbol, 8)
              << " price=" << price << " (" << price / 10000.0 << ")"
              << "\n"
              << std::endl;
}

// 'F' - Add Order with MPID Attribution.
// Identical layout to 'A' plus a trailing 4-byte Attribution (MPID) alpha field.
void process_add_order_mpid(char* msg, uint16_t len) {
    if (len < 40) {
        std::cout << "Add Order (MPID) has incomplete data " << len << "/40 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t order_ref;
    char side;
    uint32_t volume;
    char symbol[8];
    uint32_t price;
    char attribution[4]; // MPID
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    memcpy(&side, msg + offset, 1);
    ++offset;

    memcpy(&volume, msg + offset, 4);
    volume = ntohl(volume);
    offset += 4;

    memcpy(symbol, msg + offset, 8);
    offset += 8;

    memcpy(&price, msg + offset, 4);
    price = ntohl(price);
    offset += 4;

    memcpy(attribution, msg + offset, 4);
    offset += 4;

    std::cout << "\n"
              << "[Add Order MPID]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " side=" << side
              << " volume=" << volume
              << " symbol=" << std::string(symbol, 8)
              << " price=" << price << " (" << price / 10000.0 << ")"
              << " attribution=" << std::string(attribution, 4)
              << "\n"
              << std::endl;
}

// 'E' - Order Executed.
// Refers to an existing order by order_ref; reduces its displayed shares by executed_shares.
void process_executed_order(char* msg, uint16_t len) {
    if (len < 31) {
        std::cout << "Order Executed has incomplete data " << len << "/31 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    memcpy(&executed_shares, msg + offset, 4);
    executed_shares = ntohl(executed_shares);
    offset += 4;

    memcpy(&match_number, msg + offset, 8);
    match_number = __builtin_bswap64(match_number);
    offset += 8;

    std::cout << "\n"
              << "[Order Executed]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " executed_shares=" << executed_shares
              << " match_number=" << match_number
              << "\n"
              << std::endl;
}

// 'C' - Order Executed With Price.
// Like 'E' but adds a Printable flag and an execution price that may differ from the
// order's display price. Non-printable executions ("N") should be ignored for time-and-sales.
void process_executed_with_price_order(char* msg, uint16_t len) {
    if (len < 36) {
        std::cout << "Order Executed With Price has incomplete data " << len << "/36 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
    char printable; // 'Y' or 'N'
    uint32_t execution_price;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    memcpy(&executed_shares, msg + offset, 4);
    executed_shares = ntohl(executed_shares);
    offset += 4;

    memcpy(&match_number, msg + offset, 8);
    match_number = __builtin_bswap64(match_number);
    offset += 8;

    memcpy(&printable, msg + offset, 1);
    ++offset;

    memcpy(&execution_price, msg + offset, 4);
    execution_price = ntohl(execution_price);
    offset += 4;

    std::cout << "\n"
              << "[Order Executed With Price]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " executed_shares=" << executed_shares
              << " match_number=" << match_number
              << " printable=" << printable
              << " execution_price=" << execution_price << " (" << execution_price / 10000.0 << ")"
              << "\n"
              << std::endl;
}

// 'X' - Order Cancel.
// Partial cancel: removes cancelled_shares from the displayed size of an existing order.
void process_cancel_order(char* msg, uint16_t len) {
    if (len < 23) {
        std::cout << "Order Cancel has incomplete data " << len << "/23 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t order_ref;
    uint32_t cancelled_shares;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    memcpy(&cancelled_shares, msg + offset, 4);
    cancelled_shares = ntohl(cancelled_shares);
    offset += 4;

    std::cout << "\n"
              << "[Order Cancel]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " cancelled_shares=" << cancelled_shares
              << "\n"
              << std::endl;
}

// 'D' - Order Delete.
// Full cancel: the order is removed from the book entirely. Smallest of the order messages.
void process_delete_order(char* msg, uint16_t len) {
    if (len < 19) {
        std::cout << "Order Delete has incomplete data " << len << "/19 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t order_ref;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    std::cout << "\n"
              << "[Order Delete]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << "\n"
              << std::endl;
}

// 'U' - Order Replace.
// NOTE: carries TWO order-ref numbers. The original order is deleted and a new order is
// created at new_order_ref with new shares/price (loses queue priority). Side, symbol and
// MPID are NOT in this message - they must be carried over from the original order.
void process_replace_order(char* msg, uint16_t len) {
    if (len < 35) {
        std::cout << "Order Replace has incomplete data " << len << "/35 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t original_order_ref;
    uint64_t new_order_ref;
    uint32_t shares;
    uint32_t price;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&original_order_ref, msg + offset, 8);
    original_order_ref = __builtin_bswap64(original_order_ref);
    offset += 8;

    memcpy(&new_order_ref, msg + offset, 8);
    new_order_ref = __builtin_bswap64(new_order_ref);
    offset += 8;

    memcpy(&shares, msg + offset, 4);
    shares = ntohl(shares);
    offset += 4;

    memcpy(&price, msg + offset, 4);
    price = ntohl(price);
    offset += 4;

    std::cout << "\n"
              << "[Order Replace]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " original_order_ref=" << original_order_ref
              << " new_order_ref=" << new_order_ref
              << " shares=" << shares
              << " price=" << price << " (" << price / 10000.0 << ")"
              << "\n"
              << std::endl;
}

// 'P' - Trade Message (non-cross).
// NOTE: represents a match of a NON-displayable order, so it does NOT affect the book - a
// book-only consumer can ignore it. Order Reference Number is always 0 (per spec), and the
// Buy/Sell indicator has always been 'B' since 2014 regardless of resting side.
void process_trade(char* msg, uint16_t len) {
    if (len < 44) {
        std::cout << "Trade has incomplete data " << len << "/44 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    uint64_t order_ref;
    char side;
    uint32_t volume;
    char symbol[8];
    uint32_t price;
    uint64_t match_number;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&order_ref, msg + offset, 8);
    order_ref = __builtin_bswap64(order_ref);
    offset += 8;

    memcpy(&side, msg + offset, 1);
    ++offset;

    memcpy(&volume, msg + offset, 4);
    volume = ntohl(volume);
    offset += 4;

    memcpy(symbol, msg + offset, 8);
    offset += 8;

    memcpy(&price, msg + offset, 4);
    price = ntohl(price);
    offset += 4;

    memcpy(&match_number, msg + offset, 8);
    match_number = __builtin_bswap64(match_number);
    offset += 8;

    std::cout << "\n"
              << "[Trade]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " side=" << side
              << " volume=" << volume
              << " symbol=" << std::string(symbol, 8)
              << " price=" << price << " (" << price / 10000.0 << ")"
              << " match_number=" << match_number
              << "\n"
              << std::endl;
}

// 'S' - System Event.
// Signals market/feed lifecycle. event_code: 'O' start of messages, 'S' start of system hours,
// 'Q' start of market hours, 'M' end of market hours, 'E' end of system hours, 'C' end of messages.
void process_system_event(char* msg, uint16_t len) {
    if (len < 12) {
        std::cout << "System Event has incomplete data " << len << "/12 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    char event_code;
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(&event_code, msg + offset, 1);
    ++offset;

    std::cout << "\n"
              << "[System Event]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " event_code=" << event_code
              << "\n"
              << std::endl;
}

// 'H' - Stock Trading Action.
// Communicates a security's trading state. trading_state: 'H' halted, 'P' paused,
// 'Q' quotation only, 'T' trading. reason is a 4-byte code (see spec Appendix C).
void process_stock_trading_action(char* msg, uint16_t len) {
    if (len < 25) {
        std::cout << "Stock Trading Action has incomplete data " << len << "/25 bytes" << std::endl;
        return;
    }
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp {0};
    char symbol[8];
    char trading_state;
    char reserved;
    char reason[4];
    int offset {1};

    memcpy(&stock_locate, msg + offset, 2);
    stock_locate = ntohs(stock_locate);
    offset += 2;

    memcpy(&tracking_num, msg + offset, 2);
    tracking_num = ntohs(tracking_num);
    offset += 2;

    memcpy(reinterpret_cast<char*>(&timestamp) + 2, msg + offset, 6);
    timestamp = __builtin_bswap64(timestamp);
    offset += 6;

    memcpy(symbol, msg + offset, 8);
    offset += 8;

    memcpy(&trading_state, msg + offset, 1);
    ++offset;

    memcpy(&reserved, msg + offset, 1);
    ++offset;

    memcpy(reason, msg + offset, 4);
    offset += 4;

    std::cout << "\n"
              << "[Stock Trading Action]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " symbol=" << std::string(symbol, 8)
              << " trading_state=" << trading_state
              << " reason=" << std::string(reason, 4)
              << "\n"
              << std::endl;
}

void process_message(char* msg, uint16_t len) {
    // first char is the type
    char type {*msg};

    switch (type) {
        case 'A':
            process_add_order(msg, len);
            break;
        case 'F':
            process_add_order_mpid(msg, len);
            break;
        case 'E':
            process_executed_order(msg, len);
            break;
        case 'C':
            process_executed_with_price_order(msg, len);
            break;
        case 'X':
            process_cancel_order(msg, len);
            break;
        case 'D':
            process_delete_order(msg, len);
            break;
        case 'U':
            process_replace_order(msg, len);
            break;
        case 'P':
            process_trade(msg, len);
            break;
        case 'S':
            process_system_event(msg, len);
            break;
        case 'H':
            process_stock_trading_action(msg, len);
            break;
        default:
            std::cout << "This message type is not currently supported (Type: " << type << ")" << std::endl;
            break;
    }
}
