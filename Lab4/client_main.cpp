#include "client.h"
#include <iostream>

int main() {
    try {
        Client client;
        client.start();
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}