// Order book tests — replays the synthetic ITCH scenario from the data generator
// and asserts the resulting book state. No framework: a tiny CHECK harness.
//
// Build & run (from repo root):
//   g++ -std=c++20 tests/order_book_test.cpp src/order_book/order_book.cpp -o ob_test && ./ob_test

#include <array>
#include <vector>
#include <algorithm>
#include <optional>
#include <cstdint>
#include <iostream>
#include "../src/order_book/book_manager.h"

// ---- tiny test harness ---------------------------------------------------
static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond)                                                            \
    do {                                                                      \
        ++checks_run;                                                         \
        if (!(cond)) {                                                        \
            ++checks_failed;                                                  \
            std::cout << "FAIL (line " << __LINE__ << "): " << #cond << "\n"; \
        }                                                                     \
    } while (0)

// Assert an optional holds exactly `expected`.
template <typename T>
void check_opt(const std::optional<T>& got, T expected, int line) {
    ++checks_run;
    if (!got || *got != expected) {
        ++checks_failed;
        std::cout << "FAIL (line " << line << "): expected " << expected
                  << " got " << (got ? std::to_string(*got) : "nullopt") << "\n";
    }
}
#define CHECK_OPT(got, expected) check_opt((got), (uint32_t)(expected), __LINE__)

// ---- helpers to build & feed messages ------------------------------------
static std::array<char, 8> sym(const char* s) {
    std::array<char, 8> a {};
    for (int i = 0; i < 8 && s[i]; ++i) a[i] = s[i];
    return a;
}

// by-value param gives us an lvalue so BookManager::apply(T&) can bind
template <typename T>
bool feed(BookManager& m, T order) { return m.apply(order); }

static std::vector<uint32_t> sorted(std::vector<uint32_t> v) {
    std::sort(v.begin(), v.end());
    return v;
}

int main() {
    BookManager mgr;

    // ===================== AAPL (stock_locate = 1) =====================
    // Build the book
    feed(mgr, OrderAdd{ {1,0,0}, 1, Side::BUY,  100, 1500000, sym("AAPL") });
    feed(mgr, OrderAdd{ {1,0,0}, 2, Side::BUY,  200, 1495000, sym("AAPL") });
    feed(mgr, OrderAdd{ {1,0,0}, 3, Side::SELL, 150, 1505000, sym("AAPL") });
    feed(mgr, OrderAdd{ {1,0,0}, 4, Side::SELL, 50,  1510000, sym("AAPL") });

    feed(mgr, OrderExecuted{ {1,0,0}, 1, 50, 1001 });        // ref1 100 -> 50
    feed(mgr, OrderCancel{   {1,0,0}, 1, 50 });              // ref1 50 -> 0 -> deleted
    feed(mgr, OrderDelete{   {1,0,0}, 2 });                  // ref2 gone
    feed(mgr, OrderReplace{  {1,0,0}, 3, 5, 100, 1507500 }); // ref3 -> ref5 @150.75
    feed(mgr, OrderAddMPID{  {1,0,0}, 6, Side::BUY, 120, 1497500, sym("AAPL"), {'N','S','D','Q'} });

    auto aapl_opt = mgr.get_order_book(1);
    CHECK(aapl_opt.has_value());
    OrderBook* aapl = *aapl_opt;

    // Orders that should remain
    CHECK_OPT(aapl->get_order_volume(4), 50);   // untouched ask
    CHECK_OPT(aapl->get_order_volume(5), 100);  // from replace
    CHECK_OPT(aapl->get_order_volume(6), 120);  // MPID add
    // Orders that should be gone
    CHECK(!aapl->get_order_volume(1).has_value());
    CHECK(!aapl->get_order_volume(2).has_value());
    CHECK(!aapl->get_order_volume(3).has_value());

    // Price levels
    CHECK_OPT(aapl->get_price_level_volume(1497500, Side::BUY), 120);
    CHECK_OPT(aapl->get_price_level_msg_count(1497500, Side::BUY), 1);
    CHECK_OPT(aapl->get_price_level_volume(1507500, Side::SELL), 100);
    CHECK_OPT(aapl->get_price_level_volume(1510000, Side::SELL), 50);
    // Levels that must have been removed
    CHECK(!aapl->get_price_level_volume(1500000, Side::BUY).has_value());  // ref1 gone
    CHECK(!aapl->get_price_level_volume(1495000, Side::BUY).has_value());  // ref2 gone
    CHECK(!aapl->get_price_level_volume(1505000, Side::SELL).has_value()); // ref3 replaced

    CHECK(sorted(aapl->get_all_bids()) == (std::vector<uint32_t>{1497500}));
    CHECK(sorted(aapl->get_all_asks()) == (std::vector<uint32_t>{1507500, 1510000}));

    // ===================== MSFT (stock_locate = 2) =====================
    feed(mgr, OrderAdd{ {2,0,0}, 10, Side::BUY,  500, 3000000, sym("MSFT") });
    feed(mgr, OrderAdd{ {2,0,0}, 11, Side::BUY,  300, 2995000, sym("MSFT") });
    feed(mgr, OrderAdd{ {2,0,0}, 12, Side::SELL, 200, 3005000, sym("MSFT") });
    feed(mgr, OrderAdd{ {2,0,0}, 13, Side::SELL, 400, 3010000, sym("MSFT") });

    feed(mgr, OrderExecuted{          {2,0,0}, 10, 200, 2001 });            // ref10 500 -> 300
    feed(mgr, OrderExecutedWithPrice{ {2,0,0}, 11, 100, 2002, 2997500, Printable::YES }); // ref11 300 -> 200
    feed(mgr, OrderDelete{            {2,0,0}, 12 });
    feed(mgr, OrderDelete{            {2,0,0}, 13 });

    auto msft_opt = mgr.get_order_book(2);
    CHECK(msft_opt.has_value());
    OrderBook* msft = *msft_opt;

    CHECK_OPT(msft->get_order_volume(10), 300);
    CHECK_OPT(msft->get_order_volume(11), 200);
    CHECK(!msft->get_order_volume(12).has_value());
    CHECK(!msft->get_order_volume(13).has_value());

    CHECK_OPT(msft->get_price_level_volume(3000000, Side::BUY), 300);
    CHECK_OPT(msft->get_price_level_msg_count(3000000, Side::BUY), 1);
    CHECK_OPT(msft->get_price_level_volume(2995000, Side::BUY), 200);
    CHECK(msft->get_all_asks().empty());  // both asks deleted
    CHECK(sorted(msft->get_all_bids()) == (std::vector<uint32_t>{2995000, 3000000}));

    // ===================== edge cases =====================
    BookManager edge;
    // full execution should delete the order (the signed-delta bug we fixed)
    feed(edge, OrderAdd{ {9,0,0}, 100, Side::BUY, 50, 1000000, sym("TST") });
    feed(edge, OrderExecuted{ {9,0,0}, 100, 50, 1 });     // exact fill
    OrderBook* t = *edge.get_order_book(9);
    CHECK(!t->get_order_volume(100).has_value());                       // order removed
    CHECK(!t->get_price_level_volume(1000000, Side::BUY).has_value());  // level removed

    // replace on unknown ref must fail, not throw / corrupt
    OrderReplace bad_replace{ {9,0,0}, 999, 998, 10, 500000 };
    CHECK(edge.apply(bad_replace) == false);

    // ---- summary ----
    std::cout << "\n" << (checks_run - checks_failed) << "/" << checks_run << " checks passed\n";
    if (checks_failed) std::cout << checks_failed << " FAILED\n";
    return checks_failed ? 1 : 0;
}
