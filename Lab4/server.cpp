#include "server.h"
#include "protocol.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

Server::Server(int port)
    : port_(port),
      server_fd_(-1),
      threadCount_(0),
      dataSize_(0),
      k_(0),
      isProcessing_(false),
      isDone_(false) {}

Server::~Server() {
    if (computationThread_.joinable()) {
        computationThread_.join();
    }
}

vector<int> Server::parseArray(const string& values) {
    vector<int> result;
    stringstream ss(values);
    int number;

    while (ss >> number) {
        result.push_back(number);
    }

    return result;
}

string Server::buildResultString() const {
    stringstream ss;
    ss << "RESULT ";

    for (int value : C_) {
        ss << value << ' ';
    }

    return ss.str();
}

void Server::runComputation() {
    vector<int> localA;
    vector<int> localB;
    int localK;
    int localSize;
    int localThreadCount;

    {
        lock_guard<mutex> lock(dataMutex_);
        localA = A_;
        localB = B_;
        localK = k_;
        localSize = dataSize_;
        localThreadCount = threadCount_;
        C_.assign(localSize, 0);
    }

    if (localA.empty() || localB.empty() || localSize <= 0 || localThreadCount <= 0) {
        isProcessing_ = false;
        isDone_ = false;
        return;
    }

    vector<thread> workers;
    int chunkSize = localSize / localThreadCount;
    int remainder = localSize % localThreadCount;

    int startIndex = 0;

    for (int i = 0; i < localThreadCount; i++) {
        int currentChunk = chunkSize + (i < remainder ? 1 : 0);
        int endIndex = startIndex + currentChunk;

        workers.emplace_back([=, this, &localA, &localB]() {
            for (int j = startIndex; j < endIndex; j++) {
                lock_guard<mutex> lock(dataMutex_);
                C_[j] = localA[j] - localK * localB[j];
            }

            this_thread::sleep_for(chrono::milliseconds(200));
        });

        startIndex = endIndex;
    }

    for (thread& worker : workers) {
        worker.join();
    }

    isProcessing_ = false;
    isDone_ = true;

    cout << "Computation finished\n";
}

string Server::processCommand(const string& command) {
    if (command == Protocol::PING) {
        return Protocol::PONG;
    }

    if (command == Protocol::EXIT) {
        return Protocol::OK;
    }

    if (command.rfind(Protocol::CONFIG, 0) == 0) {
        stringstream ss(command);
        string cmd;
        int tempThreadCount;

        ss >> cmd >> tempThreadCount;

        if (tempThreadCount <= 0) {
            return Protocol::ERROR;
        }

        lock_guard<mutex> lock(dataMutex_);
        threadCount_ = tempThreadCount;

        cout << "Configured thread count: " << threadCount_ << '\n';
        return Protocol::OK;
    }

    if (command.rfind(Protocol::DATA, 0) == 0) {
        size_t firstSep = command.find('|');
        size_t secondSep = command.find('|', firstSep + 1);
        size_t thirdSep = command.find('|', secondSep + 1);

        if (firstSep == string::npos || secondSep == string::npos || thirdSep == string::npos) {
            return Protocol::ERROR;
        }

        string header = command.substr(0, firstSep);
        string aValues = command.substr(firstSep + 1, secondSep - firstSep - 1);
        string bValues = command.substr(secondSep + 1, thirdSep - secondSep - 1);
        string kValue = command.substr(thirdSep + 1);

        stringstream hs(header);
        string cmd;
        int tempDataSize;

        hs >> cmd >> tempDataSize;

        if (tempDataSize <= 0) {
            return Protocol::ERROR;
        }

        vector<int> tempA = parseArray(aValues);
        vector<int> tempB = parseArray(bValues);

        stringstream ks(kValue);
        int tempK;
        ks >> tempK;

        if (tempA.size() != tempDataSize || tempB.size() != tempDataSize) {
            return Protocol::ERROR;
        }

        {
            lock_guard<mutex> lock(dataMutex_);
            dataSize_ = tempDataSize;
            A_ = tempA;
            B_ = tempB;
            k_ = tempK;
            C_.clear();
        }

        isDone_ = false;
        isProcessing_ = false;

        cout << "Received DATA command\n";
        cout << "Size: " << dataSize_ << '\n';
        cout << "k: " << k_ << '\n';

        return Protocol::OK;
    }

    if (command == Protocol::START) {
        if (isProcessing_) {
            return "ALREADY_RUNNING";
        }

        {
            lock_guard<mutex> lock(dataMutex_);

            if (A_.empty() || B_.empty() || threadCount_ <= 0 || dataSize_ <= 0) {
                return Protocol::ERROR;
            }
        }

        if (computationThread_.joinable()) {
            computationThread_.join();
        }

        isProcessing_ = true;
        isDone_ = false;

        cout << "Starting computation asynchronously...\n";

        computationThread_ = thread(&Server::runComputation, this);

        return Protocol::OK;
    }

    if (command == Protocol::STATUS) {
        if (isProcessing_) {
            return "IN_PROGRESS";
        }

        if (isDone_) {
            return "DONE";
        }

        return "NOT_STARTED";
    }

    if (command == Protocol::RESULT) {
        if (isProcessing_) {
            return "IN_PROGRESS";
        }

        if (!isDone_) {
            return Protocol::ERROR;
        }

        lock_guard<mutex> lock(dataMutex_);
        return buildResultString();
    }

    return Protocol::ERROR;
}

void Server::handleClient(int clientSocket) {
    while (true) {
        char buffer[4096] = {0};
        ssize_t receivedBytes = ::recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (receivedBytes < 0) {
            perror("Receive failed");
            break;
        }

        if (receivedBytes == 0) {
            cout << "Client disconnected\n";
            break;
        }

        buffer[receivedBytes] = '\0';
        string command(buffer);

        cout << "Received command: " << command << '\n';

        string response = processCommand(command);

        if (::send(clientSocket, response.c_str(), response.size(), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (command == Protocol::EXIT) {
            cout << "Client session finished\n";
            break;
        }
    }

    ::close(clientSocket);
}

void Server::start() {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        perror("Socket creation failed");
        return;
    }

    cout << "Socket created\n";

    int opt = 1;
    if (::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        ::close(server_fd_);
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (::bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        ::close(server_fd_);
        return;
    }

    cout << "Bind successful\n";

    if (::listen(server_fd_, 5) < 0) {
        perror("Listen failed");
        ::close(server_fd_);
        return;
    }

    cout << "Server listening on port " << port_ << "...\n";

    while (true) {
        int clientSocket = ::accept(server_fd_, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        cout << "Client connected\n";
        handleClient(clientSocket);
    }
}