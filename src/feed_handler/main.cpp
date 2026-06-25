#include <iostream>
#include <feed_network.h>


int main() {
    std::cout << "Feed Handler process started" << std::endl;

    // First thing we want to do is create the multicast UDP socket
    setup_socket();

    // Then we want to call the recv function on some sort of loop
    while (true) {
        recv_market_data();
    }


    return 0;