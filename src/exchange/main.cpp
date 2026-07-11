#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>   // ntohs
#include <set>
#include "exchange_network.h"

void handle_itch_processing();

int main() {
    std::cout << "Exchange process started" << std::endl;

    setup_socket();
    handle_itch_processing();   // spawns the send + retransmission threads
    close_socket();
    return 0;
}