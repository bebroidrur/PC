#include "client.h"
#include "protocol.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

Client::Client(const string& serverIp, int port)
    : serverIp_(serverIp), port_(port) {}

void Client::start() {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port_);

    if (inet_pton(AF_INET, serverIp_.c_str(), &serverAddress.sin_addr) <= 0) {
        perror("Invalid address");
        ::close(sock);
        return;
    }

    if (::connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        ::close(sock);
        return;
    }

    cout << "Connected to server\n";

    auto sendCommand = [&](const string& command) {
        if (::send(sock, command.c_str(), command.size(), 0) < 0) {
            perror("Send failed");
            return string("ERROR");
        }

        cout << "Sent: " << command << '\n';

        char buffer[4096] = {0};
        ssize_t receivedBytes = ::recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (receivedBytes < 0) {
            perror("Receive failed");
            return string("ERROR");
        }

        buffer[receivedBytes] = '\0';
        cout << "Response: " << buffer << '\n';
        return string(buffer);
    };

    vector<int> A = {10, 20, 30, 40, 50, 60, 70, 80};
    vector<int> B = {1, 2, 3, 4, 5, 6, 7, 8};
    int k = 2;
    int threadCount = 4;

    string configCommand = Protocol::CONFIG + " " + to_string(threadCount);
    sendCommand(configCommand);

    stringstream aStream;
    stringstream bStream;

    for (int value : A) {
        aStream << value << ' ';
    }

    for (int value : B) {
        bStream << value << ' ';
    }

    string dataCommand = Protocol::DATA + " " + to_string(A.size()) +
                         "|" + aStream.str() +
                         "|" + bStream.str() +
                         "|" + to_string(k);

    sendCommand(dataCommand);
    sendCommand(Protocol::START);

    string status;
    do {
        this_thread::sleep_for(chrono::milliseconds(150));
        status = sendCommand(Protocol::STATUS);
    } while (status == "IN_PROGRESS");

    sendCommand(Protocol::RESULT);
    sendCommand(Protocol::EXIT);

    ::close(sock);
}