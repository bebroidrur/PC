#include "client.h"
#include "protocol.h"
#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

Client::Client(const string& serverIp, int port)
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

    cout << "Connected to server\n";

    string command = Protocol::PING;
    if (send(sock, command.c_str(), command.size(), 0) < 0) {
        perror("Send failed");
        close(sock);
        return;
    }

    cout << "Command sent: " << command << '\n';

    char buffer[1024] = {0};
    ssize_t receivedBytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (receivedBytes < 0) {
        perror("Receive failed");
        close(sock);
        return;
    }

    buffer[receivedBytes] = '\0';
    cout << "Response from server: " << buffer << '\n';

    string exitCommand = Protocol::EXIT;
    send(sock, exitCommand.c_str(), exitCommand.size(), 0);

    close(sock);
}