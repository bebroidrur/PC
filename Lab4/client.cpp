#include "client.h"
#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

Client::Client(const std::string& serverIp, int port)
    : serverIp_(serverIp), port_(port) {}

void Client::start() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port_);

    if (inet_pton(AF_INET, serverIp_.c_str(), &serverAddress.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    std::cout << "Connected to server\n";

    const char* message = "Hello from client";
    ssize_t sentBytes = send(sock, message, std::strlen(message), 0);

    if (sentBytes < 0) {
        perror("Send failed");
        close(sock);
        return;
    }

    std::cout << "Message sent to server\n";

    close(sock);
}