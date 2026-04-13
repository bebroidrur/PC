#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
Server::Server(int port) : port_(port), server_fd_(-1) {}

void Server::start() {
    // 1. створення сокета
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        perror("Socket creation failed");
        return;
    }

    std::cout << "Socket created\n";

    // 2. налаштування адреси
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // 3. bind
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd_);
        return;
    }

    std::cout << "Bind successful\n";

    // 4. listen
    if (listen(server_fd_, 5) < 0) {
        perror("Listen failed");
        close(server_fd_);
        return;
    }

    std::cout << "Server listening on port " << port_ << "...\n";

    // 5. accept
    while (true) {
        int client_socket = accept(server_fd_, nullptr, nullptr);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::cout << "Client connected!\n";

        // поки просто закриваємо
        close(client_socket);
    }
}