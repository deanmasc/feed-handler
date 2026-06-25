#include <iostream>
#include <exchange_network.h>


int main() {
    std::cout << "Exchange process started" << std::endl;

    // First thing we want to do is create the multicast UDP socket
    setup_socket();

    // Then we want to call some parse data function, which calls in itch_reader.cpp
    // this file reads itch binary, wraps in MoldUDP64 header, and calls send_market_data
    // defined in network.cpp


    return 0;
}