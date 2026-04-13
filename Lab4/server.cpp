#include "server.h"
#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

Server::Server(int port) : port_(port), server_fd_(-1) {}

void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        perror("Socket creation failed");
        return;
    }

    std::cout << "Socket created\n";

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd_);
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd_);
        return;
    }

    std::cout << "Bind successful\n";

    if (listen(server_fd_, 5) < 0) {
        perror("Listen failed");
        close(server_fd_);
        return;
    }

    std::cout << "Server listening on port " << port_ << "...\n";

    while (true) {
        int client_socket = accept(server_fd_, nullptr, nullptr);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::cout << "Client connected\n";

        char buffer[1024] = {0};
        ssize_t receivedBytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (receivedBytes < 0) {
            perror("Receive failed");
        } else if (receivedBytes == 0) {
            std::cout << "Client disconnected without sending data\n";
        } else {
            buffer[receivedBytes] = '\0';
            std::cout << "Received from client: " << buffer << '\n';
        }

        close(client_socket);
    }
}