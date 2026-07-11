// Gap-detection integration test.
//
// Drives the REAL feed-handler recovery path over real loopback sockets: a
// single process plays both the exchange (sends market data + answers
// retransmission requests) and the feed handler (recv_market_data + recovery
// state). We deliberately drop packet seq=2 of a 1,2,3 stream and verify that
//   - the gap is detected and a retransmission request is emitted with the
//     correct (first_seq_num, messages_lost),
//   - packet 3 is buffered (not applied) while the gap is open,
//   - once packet 2 is retransmitted, 2 is applied and 3 is drained from the
//     buffer in order, leaving packet_buffer / messages_lost empty,
//   - the resulting book matches applying 1,2,3 with no loss.
//
// It doubles as a transport smoke test: if multicast loopback isn't delivering
// on this machine the socket reads hit a 2s timeout and the state assertions
// below fail loudly (rather than the test hanging).
//
// Build & run (from repo root):
//   g++ -std=c++20 tests/gap_detection_test.cpp src/feed_handler/feed_network.cpp \
//       src/parser/parser.cpp src/order_book/order_book.cpp -o tests/gap-test && ./tests/gap-test

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <optional>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>

#include "../src/feed_handler/feed_network.h"
#include "../src/parser/parser.h"

// recv_market_data isn't declared in feed_network.h (only the public entry
// points are), so forward-declare it to match the definition in the .cpp.
std::optional<PacketDataToSend> recv_market_data(std::array<char, 1024>& buf,
                                                 uint64_t& expected_seq_num,
                                                 std::set<uint64_t>& messages_lost,
                                                 std::map<uint64_t, PacketData>& packet_buffer);

// The feed handler's socket, so we can put a recv timeout on it.
extern int sock_fd;

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

template <typename T>
static void check_eq(const std::optional<T>& got, T expected, int line) {
    ++checks_run;
    if (!got || *got != expected) {
        ++checks_failed;
        std::cout << "FAIL (line " << line << "): expected " << expected
                  << " got " << (got ? std::to_string(*got) : "nullopt") << "\n";
    }
}
#define CHECK_OPT(got, expected) check_eq((got), (uint32_t)(expected), __LINE__)

// ---- big-endian field writers --------------------------------------------
static void put16(char* p, uint16_t v) { uint16_t b = htons(v);              memcpy(p, &b, 2); }
static void put32(char* p, uint32_t v) { uint32_t b = htonl(v);              memcpy(p, &b, 4); }
static void put64(char* p, uint64_t v) { uint64_t b = __builtin_bswap64(v);  memcpy(p, &b, 8); }

// Build a full MoldUDP64 packet wrapping a single 'A' (Add Order) message.
// Layout: [0:10] session, [10:18] seq, [18:20] msg_count, [20:22] len,
//         then the 36-byte 'A' body at offset 22.
static std::array<char, 58> make_add_packet(uint64_t seq, uint16_t stock_locate,
                                            uint64_t order_ref, char side,
                                            uint32_t volume, const char* symbol,
                                            uint32_t price) {
    std::array<char, 58> pkt{};
    char* b = pkt.data();

    memcpy(b + 0, "TESTSESS01", 10);   // session (content irrelevant to recovery)
    put64(b + 10, seq);                // sequence number
    put16(b + 18, 1);                  // message count
    put16(b + 20, 36);                 // ITCH message length

    // ---- 'A' body (starts at offset 22) ----
    b[22] = 'A';                       // type
    put16(b + 23, stock_locate);       // stock_locate
    put16(b + 25, 0);                  // tracking_num
    // b[27:33] timestamp = 0 (already zeroed)
    put64(b + 33, order_ref);          // order_ref
    b[41] = side;                      // 'B' / 'S'
    put32(b + 42, volume);             // volume
    char sym[8]; memset(sym, ' ', 8);
    memcpy(sym, symbol, std::min<size_t>(8, strlen(symbol)));
    memcpy(b + 46, sym, 8);            // symbol
    put32(b + 54, price);              // price
    return pkt;
}

// ---- socket helpers ------------------------------------------------------
static void set_rcv_timeout(int fd, int seconds) {
    timeval tv{seconds, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int main() {
    // ---- feed handler side: bind + join the multicast group ----
    setup_socket();
    set_rcv_timeout(sock_fd, 2);   // don't hang forever if delivery fails

    // ---- exchange TX: multicast sender (loops back on 127.0.0.1) ----
    int tx = socket(AF_INET, SOCK_DGRAM, 0);
    int loop = 1;
    setsockopt(tx, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    in_addr iface{};
    inet_pton(AF_INET, "127.0.0.1", &iface.s_addr);
    setsockopt(tx, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface));
    sockaddr_in grp{};
    grp.sin_family = AF_INET;
    grp.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_IP_ADDR, &grp.sin_addr);

    // ---- exchange RX: listens for retransmission requests on 30001 ----
    int ex = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(ex, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in rin{};
    rin.sin_family = AF_INET;
    rin.sin_port = htons(RETRANSMISSION_PORT);
    inet_pton(AF_INET, RETRANSMISSION_IP_ADDR, &rin.sin_addr);
    bind(ex, (sockaddr*)&rin, sizeof(rin));
    set_rcv_timeout(ex, 2);

    auto send_pkt = [&](const std::array<char, 58>& p) {
        sendto(tx, p.data(), p.size(), 0, (sockaddr*)&grp, sizeof(grp));
    };

    // Three adds on stock_locate 1. seq 2 is the one we'll "drop".
    auto pkt1 = make_add_packet(1, 1, 101, 'B', 500, "AAAA", 1500000);
    auto pkt2 = make_add_packet(2, 1, 102, 'S', 300, "AAAA", 1510000);
    auto pkt3 = make_add_packet(3, 1, 103, 'B', 200, "AAAA", 1495000);

    // ---- feed handler recovery state (owned here so we can inspect it) ----
    uint64_t expected_seq_num{1};
    std::set<uint64_t> messages_lost;
    std::map<uint64_t, PacketData> packet_buffer;
    std::array<char, 1024> buf;
    BookManager mgr;

    // Mirrors the body of handle_recv_market_data: recv one packet, apply it if
    // it came back ready, then drain the buffer if we're fully caught up.
    auto drive_once = [&]() {
        auto pds = recv_market_data(buf, expected_seq_num, messages_lost, packet_buffer);
        if (pds) {
            process_message(pds->buf_data, pds->msg_len, mgr);
            if (messages_lost.empty() && !packet_buffer.empty()) {
                for (auto& [seq, pd] : packet_buffer) {
                    process_message(pd.data.data() + 22, pd.bytes - 22, mgr);
                }
                packet_buffer.clear();
            }
        }
    };

    // ===================== step 1: in-order packet 1 =====================
    send_pkt(pkt1);
    send_pkt(pkt3);   // 2 is dropped; 3 arrives out of order
    drive_once();     // reads pkt1

    CHECK(expected_seq_num == 2);
    CHECK(messages_lost.empty());
    CHECK(packet_buffer.empty());
    {
        auto ob = mgr.get_order_book(1);
        CHECK(ob.has_value());
        CHECK_OPT((*ob)->get_order_volume(101), 500);
        CHECK(!(*ob)->get_order_volume(102).has_value());  // not seen yet
    }

    // ===================== step 2: gap on packet 3 =====================
    drive_once();     // reads pkt3 -> detects gap, buffers it, sends request

    CHECK(messages_lost.size() == 1 && messages_lost.count(2) == 1);
    CHECK(packet_buffer.size() == 1 && packet_buffer.count(3) == 1);
    CHECK(expected_seq_num == 4);
    {
        auto ob = mgr.get_order_book(1);
        CHECK(!(*ob)->get_order_volume(103).has_value());  // buffered, not applied
        CHECK(!(*ob)->get_order_volume(102).has_value());
    }

    // ---- exchange receives the retransmission request ----
    char req[32];
    ssize_t rq = recvfrom(ex, req, sizeof(req), 0, nullptr, nullptr);
    CHECK(rq == 10);   // 8-byte seq + 2-byte count
    if (rq == 10) {
        uint64_t first_seq_num;
        uint16_t msgs_lost;
        memcpy(&first_seq_num, &req[0], 8);   // FH writes these in host order
        memcpy(&msgs_lost, &req[8], 2);
        CHECK(first_seq_num == 2);
        CHECK(msgs_lost == 1);
    }

    // ===================== step 3: retransmit packet 2 =====================
    send_pkt(pkt2);   // exchange resends the missing packet on multicast
    drive_once();     // reads pkt2 -> applied, then buffer (pkt3) drains

    CHECK(messages_lost.empty());
    CHECK(packet_buffer.empty());
    CHECK(expected_seq_num == 4);

    // ---- final book: same as applying 1,2,3 with no loss ----
    {
        auto ob = mgr.get_order_book(1);
        CHECK(ob.has_value());
        OrderBook* book = *ob;
        CHECK_OPT(book->get_order_volume(101), 500);
        CHECK_OPT(book->get_order_price(101), 1500000);
        CHECK_OPT(book->get_order_volume(102), 300);   // the dropped+recovered one
        CHECK_OPT(book->get_order_price(102), 1510000);
        CHECK_OPT(book->get_order_volume(103), 200);
        CHECK_OPT(book->get_order_price(103), 1495000);

        auto bids = book->get_all_bids();
        std::sort(bids.begin(), bids.end());
        CHECK(bids == (std::vector<uint32_t>{1495000, 1500000}));
        auto asks = book->get_all_asks();
        std::sort(asks.begin(), asks.end());
        CHECK(asks == (std::vector<uint32_t>{1510000}));
    }

    close(tx);
    close(ex);
    close_socket();

    std::cout << "\n" << (checks_run - checks_failed) << "/" << checks_run << " checks passed\n";
    if (checks_failed) std::cout << checks_failed << " FAILED\n";
    return checks_failed ? 1 : 0;
}
