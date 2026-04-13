#include "server.h"
#include <iostream>

int main() {
    try {
        Server server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}