#include <iostream>
#include <string>
#include <cstdint>      // uintN_t
#include "read_data_types.h"
#include "../order_book/data_models.h"

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
    std::array<char, 8> symbol;
    uint32_t price;
    int offset {1};

    // Assigning all fields

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);
    read_char(side, offset, msg);
    read_int(volume, offset, msg);
    read_array(symbol, offset, msg);
    read_int(price, offset, msg);

    std::cout << "\n"
              << "[Add Order]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " side=" << side
              << " volume=" << volume
              << " symbol=" << std::string(symbol.data(), 8)
              << " price=" << price << " (" << price / 10000.0 << ")"
              << "\n"
              << std::endl;

    // Now we create the Add order class or struct

    OrderAdd order {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                    static_cast<Side>(side),
                    volume,
                    price,
                    symbol
                };
        
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
    std::array<char, 8> symbol;
    uint32_t price;
    std::array<char, 4> attribution;
    int offset {1};

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);
    read_char(side, offset, msg);
    read_int(volume, offset, msg);
    read_array(symbol, offset, msg);
    read_int(price, offset, msg);
    read_array(attribution, offset, msg);

    std::cout << "\n"
              << "[Add Order MPID]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " side=" << side
              << " volume=" << volume
              << " symbol=" << std::string(symbol.data(), 8)
              << " price=" << price << " (" << price / 10000.0 << ")"
              << " attribution=" << std::string(attribution.data(), 4)
              << "\n"
              << std::endl;

    OrderAddMPID order {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                    static_cast<Side>(side),
                    volume,
                    price,
                    symbol,
                    attribution
                };
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

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);
    read_int(executed_shares, offset, msg);
    read_int(match_number, offset, msg);

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

    OrderExecuted order {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                    executed_shares,
                    match_number,
                };
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

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);
    read_int(executed_shares, offset, msg);
    read_int(match_number, offset, msg);
    read_char(printable, offset, msg);
    read_int(execution_price, offset, msg);

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

    OrderExecutedWithPrice order {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                    executed_shares,
                    match_number,
                    execution_price,
                    static_cast<Printable>(printable),
                };
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

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);
    read_int(cancelled_shares, offset, msg);

    std::cout << "\n"
              << "[Order Cancel]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " cancelled_shares=" << cancelled_shares
              << "\n"
              << std::endl;

    OrderCancel order {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                    cancelled_shares,
                };
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

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);

    std::cout << "\n"
              << "[Order Delete]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << "\n"
              << std::endl;

    OrderDelete order {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                };
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

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(original_order_ref, offset, msg);
    read_int(new_order_ref, offset, msg);
    read_int(shares, offset, msg);
    read_int(price, offset, msg);

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

    OrderReplace order {
                    {stock_locate, tracking_num, timestamp,},
                    original_order_ref,
                    new_order_ref,
                    shares,
                    price,
                };
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
    std::array<char, 8> symbol;
    uint32_t price;
    uint64_t match_number;
    int offset {1};

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_int(order_ref, offset, msg);
    read_char(side, offset, msg);
    read_int(volume, offset, msg);
    read_array(symbol, offset, msg);
    read_int(price, offset, msg);
    read_int(match_number, offset, msg);

    std::cout << "\n"
              << "[Trade]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " order_ref=" << order_ref
              << " side=" << side
              << " volume=" << volume
              << " symbol=" << std::string(symbol.data(), 8)
              << " price=" << price << " (" << price / 10000.0 << ")"
              << " match_number=" << match_number
              << "\n"
              << std::endl;

    Trade message {
                    {stock_locate, tracking_num, timestamp,},
                    order_ref,
                    static_cast<Side>(side),
                    volume,
                    price,
                    match_number,
                    symbol
                };
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

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_char(event_code, offset, msg);

    std::cout << "\n"
              << "[System Event]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " event_code=" << event_code
              << "\n"
              << std::endl;

    SystemEvent message {
                    {stock_locate, tracking_num, timestamp,},
                    static_cast<EventCode>(event_code),
                };
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
    std::array<char, 8> symbol;
    char trading_state;
    char reserved;
    std::array<char, 4> reason;
    int offset {1};

    read_header(stock_locate, tracking_num, timestamp, offset, msg);
    read_array(symbol, offset, msg);
    read_char(trading_state, offset, msg);
    read_char(reserved, offset, msg);
    read_array(reason, offset, msg);

    std::cout << "\n"
              << "[Stock Trading Action]"
              << " stock_locate=" << stock_locate
              << " tracking_num=" << tracking_num
              << " timestamp=" << timestamp
              << " symbol=" << std::string(symbol.data(), 8)
              << " trading_state=" << trading_state
              << " reason=" << std::string(reason.data(), 4)
              << "\n"
              << std::endl;

    StockTradingAction message {
                    {stock_locate, tracking_num, timestamp,},
                    static_cast<TradingState>(trading_state),
                    symbol,
                    reason,
                };
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
