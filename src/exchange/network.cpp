#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <network.h>

int sock_fd;
sockaddr_in dest {};


void setup_socket() {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // destination: the multicast group
    dest.sin_family = AF_INET;
    dest.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_IP_ADDR, &dest.sin_addr);
}


// dummy function for now
void send_market_data(char data[]) {
    // This function will be called with MoldUDP64 wrapped market data to send to the multicast UDP
    sendto(sock_fd, data, sizeof(data), 0, (sockaddr*)&dest, sizeof(dest));
}


void close_socket() {
    close(sock_fd);
}