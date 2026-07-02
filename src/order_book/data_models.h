#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <cstdint>      // uintN_t
#include <array>        // std::array

enum class Side : char {
    BUY = 'B',
    SELL = 'S',
};

enum class Printable : char {
    YES = 'Y',
    NO = 'N',
};

enum class EventCode : char {
    START_MESSAGES = 'O',
    START_SYSTEM_HOURS = 'S',
    START_MARKET_HOURS = 'Q',
    END_MARKET_HOURS = 'E',
    END_MESSAGES = 'C',
};

enum class TradingState : char {
    HALTED = 'H',
    PAUSED = 'P',
    QUOTATION_ONLY = 'Q',
    TRADING = 'T',
};

struct MessageHeader {
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint64_t timestamp;
};

struct OrderAdd {
    MessageHeader msg_header;
    uint64_t order_ref;
    Side side;
    uint32_t volume;
    uint32_t price;
    std::array<char, 8> symbol;
};

struct OrderAddMPID {
    MessageHeader msg_header;
    uint64_t order_ref;
    Side side;
    uint32_t volume;
    uint32_t price;
    std::array<char, 8> symbol;
    std::array<char, 4> attribution;
};

// 'E' - Order Executed
struct OrderExecuted {
    MessageHeader msg_header;
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
};

// 'C' - Order Executed With Price
struct OrderExecutedWithPrice {
    MessageHeader msg_header;
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
    uint32_t execution_price;
    Printable printable;
};

// 'X' - Order Cancel (partial)
struct OrderCancel {
    MessageHeader msg_header;
    uint64_t order_ref;
    uint32_t cancelled_shares;
};

// 'D' - Order Delete (full)
struct OrderDelete {
    MessageHeader msg_header;
    uint64_t order_ref;
};

// 'U' - Order Replace. Two refs: original is removed, new one created with new shares/price.
struct OrderReplace {
    MessageHeader msg_header;
    uint64_t original_order_ref;
    uint64_t new_order_ref;
    uint32_t shares;
    uint32_t price;
};

// 'P' - Trade (non-cross). Non-displayable match; does not affect the book.
struct Trade {
    MessageHeader msg_header;
    uint64_t order_ref;
    Side side;
    uint32_t volume;
    uint32_t price;
    uint64_t match_number;
    std::array<char, 8> symbol;
};

// 'S' - System Event
struct SystemEvent {
    MessageHeader msg_header;
    EventCode event_code;
};

// 'H' - Stock Trading Action
struct StockTradingAction {
    MessageHeader msg_header;
    TradingState trading_state;
    std::array<char, 8> symbol;
    std::array<char, 4> reason;
};

#endif