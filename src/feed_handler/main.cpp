#include <iostream>
#include "feed_network.h"

int main() {
    std::cout << "Feed Handler process started" << std::endl;

    // First thing we want to do is create the multicast UDP socket
    setup_socket();

    // Manages order books for each locate
    BookManager book_manager;

    // Then we want to call the recv function on some sort of loop
    handle_recv_market_data(book_manager);


    return 0;
}