#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

using namespace std;

class Server {
public:
    Server(int port = 8080);
    ~Server();

    void start();

private:
    struct ClientSession {
        int socket;
        int threadCount;
        int dataSize;
        int k;

        vector<int> A;
        vector<int> B;
        vector<int> C;

        atomic<bool> isProcessing;
        atomic<bool> isDone;

        thread computationThread;
        mutex dataMutex;

        ClientSession(int clientSocket)
            : socket(clientSocket),
              threadCount(0),
              dataSize(0),
              k(0),
              isProcessing(false),
              isDone(false) {}
    };

    int port_;
    int server_fd_;

    void handleClient(shared_ptr<ClientSession> session);
    string processCommand(shared_ptr<ClientSession> session, const string& command);
    vector<int> parseArray(const string& values);
    void runComputation(shared_ptr<ClientSession> session);
    string buildResultString(shared_ptr<ClientSession> session) const;
};

#endif