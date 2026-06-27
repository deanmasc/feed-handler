#include <fstream>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#define htobe64(x) __builtin_bswap64(x)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Write a 2-byte big-endian length prefix then the message bytes
void write_message(std::ofstream& file, const char* msg, uint16_t len) {
    uint16_t len_be = htons(len);
    file.write(reinterpret_cast<const char*>(&len_be), 2);
    file.write(msg, len);
}

// Copy a 6-byte (48-bit) big-endian timestamp from a uint64_t
// We store the value in the upper 6 bytes of a big-endian uint64_t,
// then skip the 2 most-significant bytes (which are zero for any realistic nanosecond value)
static void put_ts(char* buf, int offset, uint64_t ns) {
    uint64_t be = htobe64(ns);
    memcpy(buf + offset, reinterpret_cast<char*>(&be) + 2, 6);
}

static void put_u16(char* buf, int offset, uint16_t v) {
    uint16_t be = htons(v);
    memcpy(buf + offset, &be, 2);
}

static void put_u32(char* buf, int offset, uint32_t v) {
    uint32_t be = htonl(v);
    memcpy(buf + offset, &be, 4);
}

static void put_u64(char* buf, int offset, uint64_t v) {
    uint64_t be = htobe64(v);
    memcpy(buf + offset, &be, 8);
}

// Write an 8-byte space-padded alpha stock symbol
static void put_stock(char* buf, int offset, const char* sym) {
    char tmp[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
    for (int i = 0; i < 8 && sym[i] != '\0'; ++i) tmp[i] = sym[i];
    memcpy(buf + offset, tmp, 8);
}

// ---------------------------------------------------------------------------
// 1.1  System Event Message  (12 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'S'
//   1      2   Stock Locate  (always 0)
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      1   Event Code
void write_system_event(std::ofstream& file, uint64_t ts_ns, char event_code) {
    char msg[12] = {};
    msg[0] = 'S';
    put_u16(msg, 1, 0);          // stock locate = 0
    put_u16(msg, 3, 0);          // tracking number
    put_ts (msg, 5, ts_ns);
    msg[11] = event_code;
    write_message(file, msg, 12);
}

// ---------------------------------------------------------------------------
// 1.3.1  Add Order – No MPID Attribution  (36 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'A'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Order Reference Number
//  19      1   Buy/Sell Indicator  'B' or 'S'
//  20      4   Shares
//  24      8   Stock
//  32      4   Price (4 implied decimals, so $150.00 = 1500000)
void write_add_order(std::ofstream& file,
                     uint64_t ts_ns,
                     uint16_t stock_locate,
                     uint64_t order_ref,
                     char     side,        // 'B' or 'S'
                     uint32_t shares,
                     const char* stock,
                     uint32_t price)       // integer ticks (4 decimal places)
{
    char msg[36] = {};
    msg[0] = 'A';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, order_ref);
    msg[19] = side;
    put_u32(msg, 20, shares);
    put_stock(msg, 24, stock);
    put_u32(msg, 32, price);
    write_message(file, msg, 36);
}

// ---------------------------------------------------------------------------
// 1.4.1  Order Executed Message  (31 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'E'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Order Reference Number
//  19      4   Executed Shares
//  23      8   Match Number
void write_order_executed(std::ofstream& file,
                          uint64_t ts_ns,
                          uint16_t stock_locate,
                          uint64_t order_ref,
                          uint32_t exec_shares,
                          uint64_t match_number)
{
    char msg[31] = {};
    msg[0] = 'E';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, order_ref);
    put_u32(msg, 19, exec_shares);
    put_u64(msg, 23, match_number);
    write_message(file, msg, 31);
}

// ---------------------------------------------------------------------------
// 1.4.2  Order Executed With Price Message  (36 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'C'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Order Reference Number
//  19      4   Executed Shares
//  23      8   Match Number
//  31      1   Printable  'Y' or 'N'
//  32      4   Execution Price
void write_order_executed_with_price(std::ofstream& file,
                                     uint64_t ts_ns,
                                     uint16_t stock_locate,
                                     uint64_t order_ref,
                                     uint32_t exec_shares,
                                     uint64_t match_number,
                                     char     printable,
                                     uint32_t exec_price)
{
    char msg[36] = {};
    msg[0] = 'C';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, order_ref);
    put_u32(msg, 19, exec_shares);
    put_u64(msg, 23, match_number);
    msg[31] = printable;
    put_u32(msg, 32, exec_price);
    write_message(file, msg, 36);
}

// ---------------------------------------------------------------------------
// 1.4.3  Order Cancel Message  (23 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'X'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Order Reference Number
//  19      4   Cancelled Shares
void write_order_cancel(std::ofstream& file,
                        uint64_t ts_ns,
                        uint16_t stock_locate,
                        uint64_t order_ref,
                        uint32_t cancelled_shares)
{
    char msg[23] = {};
    msg[0] = 'X';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, order_ref);
    put_u32(msg, 19, cancelled_shares);
    write_message(file, msg, 23);
}

// ---------------------------------------------------------------------------
// 1.4.4  Order Delete Message  (19 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'D'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Order Reference Number
void write_order_delete(std::ofstream& file,
                        uint64_t ts_ns,
                        uint16_t stock_locate,
                        uint64_t order_ref)
{
    char msg[19] = {};
    msg[0] = 'D';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, order_ref);
    write_message(file, msg, 19);
}

// ---------------------------------------------------------------------------
// 1.4.5  Order Replace Message  (35 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'U'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Original Order Reference Number
//  19      8   New Order Reference Number
//  27      4   Shares
//  31      4   Price
void write_order_replace(std::ofstream& file,
                         uint64_t ts_ns,
                         uint16_t stock_locate,
                         uint64_t orig_order_ref,
                         uint64_t new_order_ref,
                         uint32_t shares,
                         uint32_t price)
{
    char msg[35] = {};
    msg[0] = 'U';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, orig_order_ref);
    put_u64(msg, 19, new_order_ref);
    put_u32(msg, 27, shares);
    put_u32(msg, 31, price);
    write_message(file, msg, 35);
}

// ---------------------------------------------------------------------------
// 1.5.1  Trade Message (Non-Cross)  (44 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'P'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Order Reference Number  (0 per spec post-2010)
//  19      1   Buy/Sell Indicator  (always 'B' per spec post-2014)
//  20      4   Shares
//  24      8   Stock
//  32      4   Price
//  36      8   Match Number
void write_trade(std::ofstream& file,
                 uint64_t ts_ns,
                 uint16_t stock_locate,
                 uint32_t shares,
                 const char* stock,
                 uint32_t price,
                 uint64_t match_number)
{
    char msg[44] = {};
    msg[0] = 'P';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_u64(msg, 11, 0);          // order ref = 0 per spec
    msg[19] = 'B';                // always 'B' per spec post-2014
    put_u32(msg, 20, shares);
    put_stock(msg, 24, stock);
    put_u32(msg, 32, price);
    put_u64(msg, 36, match_number);
    write_message(file, msg, 44);
}

// ---------------------------------------------------------------------------
// 1.2.2  Stock Trading Action  (25 bytes)
// ---------------------------------------------------------------------------
// Offset  Len  Field
//   0      1   Message Type  'H'
//   1      2   Stock Locate
//   3      2   Tracking Number
//   5      6   Timestamp
//  11      8   Stock
//  19      1   Trading State  'T' = trading
//  20      1   Reserved
//  21      4   Reason
void write_stock_trading_action(std::ofstream& file,
                                uint64_t ts_ns,
                                uint16_t stock_locate,
                                const char* stock,
                                char trading_state)  // 'T' = trading, 'H' = halted
{
    char msg[25] = {};
    msg[0] = 'H';
    put_u16(msg,  1, stock_locate);
    put_u16(msg,  3, 0);
    put_ts (msg,  5, ts_ns);
    put_stock(msg, 11, stock);
    msg[19] = trading_state;
    msg[20] = ' ';               // reserved
    memcpy(msg + 21, "    ", 4); // reason = spaces (not available)
    write_message(file, msg, 25);
}

// ---------------------------------------------------------------------------
// main – generates a small but realistic synthetic ITCH session
//
// Two instruments:
//   Stock locate 1 = AAPL
//   Stock locate 2 = MSFT
//
// Scenario (order reference numbers are unique across the session):
//
//   Start of messages / system hours / market hours
//   Trading action spin (both symbols released for trading)
//
//   --- AAPL book build ---
//   Add bid  100 @ $150.00  ref=1
//   Add bid  200 @ $149.50  ref=2
//   Add ask  150 @ $150.50  ref=3
//   Add ask   50 @ $151.00  ref=4
//   Execute 50 shares of ref=1 (partial fill)       match=1001
//   Cancel  remaining 50 shares of ref=1 (partial)
//   Delete  ref=2 entirely
//   Replace ref=3: new ref=5, 100 shares @ $150.75
//   Trade message (non-display match)               match=1002
//
//   --- MSFT book build ---
//   Add bid  500 @ $300.00  ref=10
//   Add bid  300 @ $299.50  ref=11
//   Add ask  200 @ $300.50  ref=12
//   Add ask  400 @ $301.00  ref=13
//   Execute 200 shares of ref=10 (full fill)        match=2001
//   Execute with price 100 shares of ref=11 @ $299.75 match=2002
//   Delete  ref=12
//   Delete  ref=13
//
//   End of market hours / system hours / end of messages
// ---------------------------------------------------------------------------
int main() {
    std::ofstream file("../data/test.NASDAQ_ITCH50", std::ios::binary);
    if (!file) {
        return 1;
    }

    // Nanoseconds since midnight timestamps – spread across a trading day
    // 9:30:00.000 = 34200 * 1e9
    const uint64_t T = 34200ULL * 1'000'000'000ULL;
    uint64_t ts = T;
    auto tick = [&](uint64_t ms = 100) -> uint64_t {
        ts += ms * 1'000'000ULL;
        return ts;
    };

    // -----------------------------------------------------------------------
    // Session open
    // -----------------------------------------------------------------------
    write_system_event(file, tick(), 'O');   // Start of Messages
    write_system_event(file, tick(), 'S');   // Start of System Hours
    write_system_event(file, tick(), 'Q');   // Start of Market Hours

    // Trading action spin – both symbols released
    write_stock_trading_action(file, tick(), 1, "AAPL", 'T');
    write_stock_trading_action(file, tick(), 2, "MSFT", 'T');

    // -----------------------------------------------------------------------
    // AAPL  (stock_locate = 1)
    // Price 4 format: $150.00 = 1500000, $149.50 = 1495000, etc.
    // -----------------------------------------------------------------------

    // Build the book
    write_add_order(file, tick(), 1, 1, 'B', 100, "AAPL", 1500000); // bid 100@150.00
    write_add_order(file, tick(), 1, 2, 'B', 200, "AAPL", 1495000); // bid 200@149.50
    write_add_order(file, tick(), 1, 3, 'S', 150, "AAPL", 1505000); // ask 150@150.50
    write_add_order(file, tick(), 1, 4, 'S',  50, "AAPL", 1510000); // ask  50@151.00

    // Partial execution: 50 of ref=1 (100 shares bid at 150.00)
    write_order_executed(file, tick(), 1, 1, 50, 1001);

    // Partially cancel remaining 50 of ref=1
    write_order_cancel(file, tick(), 1, 1, 50);

    // Delete ref=2 (bid 200@149.50) entirely
    write_order_delete(file, tick(), 1, 2);

    // Replace ref=3 (ask 150@150.50) -> new ref=5, 100 shares @ 150.75
    write_order_replace(file, tick(), 1, 3, 5, 100, 1507500);

    // Non-display trade in AAPL
    write_trade(file, tick(), 1, 75, "AAPL", 1500000, 1002);

    // -----------------------------------------------------------------------
    // MSFT  (stock_locate = 2)
    // $300.00 = 3000000, $299.50 = 2995000, etc.
    // -----------------------------------------------------------------------

    write_add_order(file, tick(), 2, 10, 'B', 500, "MSFT", 3000000); // bid 500@300.00
    write_add_order(file, tick(), 2, 11, 'B', 300, "MSFT", 2995000); // bid 300@299.50
    write_add_order(file, tick(), 2, 12, 'S', 200, "MSFT", 3005000); // ask 200@300.50
    write_add_order(file, tick(), 2, 13, 'S', 400, "MSFT", 3010000); // ask 400@301.00

    // Full execution of ref=10 (bid 500@300.00) – executing 200 shares
    write_order_executed(file, tick(), 2, 10, 200, 2001);

    // Partial execution with price improvement: 100 of ref=11 @ 299.75
    write_order_executed_with_price(file, tick(), 2, 11, 100, 2002, 'Y', 2997500);

    // Clean up remaining ask side
    write_order_delete(file, tick(), 2, 12);
    write_order_delete(file, tick(), 2, 13);

    // -----------------------------------------------------------------------
    // Session close
    // -----------------------------------------------------------------------
    write_system_event(file, tick(), 'M');   // End of Market Hours
    write_system_event(file, tick(), 'E');   // End of System Hours
    write_system_event(file, tick(), 'C');   // End of Messages

    return 0;
}