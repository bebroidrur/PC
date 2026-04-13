#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <atomic>

using namespace std;

class Server {
public:
    Server(int port = 8080);
    void start();

private:
    int port_;
    int server_fd_;

    int threadCount_;
    int dataSize_;
    int k_;

    vector<int> A_;
    vector<int> B_;
    vector<int> C_;

    atomic<bool> isProcessing_;
    atomic<bool> isDone_;

    void handleClient(int clientSocket);
    string processCommand(const string& command);
    vector<int> parseArray(const string& values);
    void startComputation();
    string buildResultString() const;
};

#endif