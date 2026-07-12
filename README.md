# feed-handler

A C++20 feed handler that receives NASDAQ TotalView-ITCH 5.0 market data over UDP multicast, parses binary messages, and maintains an in-memory order book — covering the full path from raw packets to correct book state, including gap detection and retransmission recovery.

## Architecture

Two processes communicate over loopback UDP multicast, simulating an exchange feed:

- **Exchange** (`src/exchange/`) — reads a raw ITCH 5.0 file, batches messages into MoldUDP64 packets, and multicasts them. Buffers recently sent packets (bounded, oldest evicted first) and serves retransmission requests from a second thread over a dedicated request/response port.
- **Feed handler** (`src/feed_handler/`) — joins the multicast group, tracks the expected MoldUDP64 sequence number, parses incoming ITCH messages, and applies them to an in-memory order book. On detecting a gap it requests retransmission and buffers out-of-order packets until the gap is filled, then replays them in sequence.

## Wire format

Each UDP packet is a MoldUDP64 frame: a 20-byte header (10-byte session, 8-byte sequence number, 2-byte message count) followed by one or more `[2-byte length][ITCH message]` entries, packed to fill the packet. The sequence number identifies the first message in the packet; the next packet's sequence number is `seq + message_count`.

## Gap detection & recovery

The feed handler tracks `expected_seq_num`. On each received packet:
- **Matches expected** — process now (or buffer if a gap is still open), advance `expected_seq_num` by the packet's message count.
- **Is a pending retransmission** — apply/buffer depending on whether it's the next contiguous piece; erase its message numbers from the loss set.
- **Is ahead of expected** — a gap: record the missing message-number range, send a retransmission request `(first_seq_num, messages_lost)` to the exchange, and buffer the packet that revealed the gap.
- **Is behind expected** — a duplicate; dropped.

Once the loss set empties, any buffered packets drain in ascending sequence order before normal processing resumes.

## ITCH messages supported

Order Add (`A`/`F`), Order Executed (`E`/`C`), Order Cancel (`X`), Order Delete (`D`), Order Replace (`U`), Trade (`P`), System Event (`S`), Stock Trading Action (`H`).

## Order book

Per-`stock_locate` order books (`BookManager`) track individual orders and aggregate price-level volume/message counts per side. Prices are fixed-point (4 decimals), timestamps are 48-bit nanoseconds-since-midnight.

## Project structure

```
src/
  exchange/        exchange-side networking, ITCH replay, retransmission service
  feed_handler/     feed-handler networking, gap detection/recovery
  parser/           binary ITCH message decoding
  order_book/       order book + data models
  data_generator/    synthetic ITCH file generator (for testing)
tests/
  order_book_test.cpp        order book logic against a synthetic scenario
  gap_detection_test.cpp     end-to-end gap detection/recovery over real sockets
```

## Build & run

Exchange:
```
g++ -std=c++20 -pthread src/exchange/main.cpp src/exchange/exchange_network.cpp \
    src/exchange/itch_handler.cpp -o exchange_bin
```

Feed handler:
```
g++ -std=c++20 src/feed_handler/main.cpp src/feed_handler/feed_network.cpp \
    src/parser/parser.cpp src/order_book/order_book.cpp -o feed_bin
```

Start the feed handler first (it must be joined to the multicast group and listening for retransmission requests before the exchange sends), then the exchange, in separate terminals.

## Scope

Stops at the order book — fan-out to downstream consumers is out of scope.
