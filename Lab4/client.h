#ifndef CLIENT_H
#define CLIENT_H
#include <string>

class Client {
public:
    Client(const std::string& serverIp = "127.0.0.1", int port = 8080);
    void start();

private:
    std::string serverIp_;
    int port_;
};
#endif //CLIENT_H
