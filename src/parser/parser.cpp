#include <memory>
#include <iostream>

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

void process_executed_order(char* msg, uint16_t len) {

}

void process_cancel_order(char* msg, uint16_t len) {

}

void process_delete_order(char* msg, uint16_t len) {

}

void process_replace_order(char* msg, uint16_t len) {

}

void process_message(char* msg, uint16_t len) {
    // first char is the type
    char type {*msg};

    switch (type) {
        case 'A':
            process_add_order(msg, len);
            break;
        // case 'E':
        //     process_executed_order(msg, len);
        //     break;
        // case 'X':
        //     process_cancel_order(msg, len);
        //     break;
        // case 'D':
        //     process_delete_order(msg, len);
        //     break;
        // case 'U':
        //     process_replace_order(msg, len);
        //     break;
        // case 'F':
        // case 'C':
        // case 'P':
        // case 'S':
        // case 'H':
        default:
            std::cout << "This message type is not currently supported (Type: " << type << ")" << std::endl;
            break;
    }
}