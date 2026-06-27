#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>   // ntohs
#include <set>
#include "exchange_network.h"

void read_and_send_itch_data();

int main() {
    std::cout << "Exchange process started" << std::endl;

    // First thing we want to do is create the multicast UDP socket
    setup_socket();
    read_and_send_itch_data();
    close_socket();
    return 0;
}