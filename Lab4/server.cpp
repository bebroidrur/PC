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

Server::Server(int port) : port_(port), server_fd_(-1) {}

Server::~Server() {
    ::close(server_fd_);
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

string Server::buildResultString(shared_ptr<ClientSession> session) const {
    lock_guard<mutex> lock(session->dataMutex);

    stringstream ss;
    ss << "RESULT ";

    for (int value : session->C) {
        ss << value << ' ';
    }

    return ss.str();
}

void Server::runComputation(shared_ptr<ClientSession> session) {
    vector<int> localA;
    vector<int> localB;
    int localK;
    int localSize;
    int localThreadCount;

    {
        lock_guard<mutex> lock(session->dataMutex);
        localA = session->A;
        localB = session->B;
        localK = session->k;
        localSize = session->dataSize;
        localThreadCount = session->threadCount;
        session->C.assign(localSize, 0);
    }

    if (localA.empty() || localB.empty() || localSize <= 0 || localThreadCount <= 0) {
        session->isProcessing = false;
        session->isDone = false;
        return;
    }

    vector<thread> workers;
    int chunkSize = localSize / localThreadCount;
    int remainder = localSize % localThreadCount;

    int startIndex = 0;

    for (int i = 0; i < localThreadCount; i++) {
        int currentChunk = chunkSize + (i < remainder ? 1 : 0);
        int endIndex = startIndex + currentChunk;

        workers.emplace_back([this, session, startIndex, endIndex, localK, &localA, &localB]() {
            for (int j = startIndex; j < endIndex; j++) {
                session->C[j] = localA[j] - localK * localB[j];
            }

            this_thread::sleep_for(chrono::milliseconds(200));
        });

        startIndex = endIndex;
    }

    for (thread& worker : workers) {
        worker.join();
    }

    session->isProcessing = false;
    session->isDone = true;

    cout << "Computation finished for client socket " << session->socket << '\n';
}

string Server::processCommand(shared_ptr<ClientSession> session, const string& command) {
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

        if (ss.fail() || tempThreadCount <= 0) {
            return Protocol::ERROR_INVALID_CONFIG;
        }

        {
            lock_guard<mutex> lock(session->dataMutex);
            session->threadCount = tempThreadCount;
        }

        cout << "Configured thread count for client " << session->socket
             << ": " << tempThreadCount << '\n';
        return Protocol::OK;
    }

    if (command.rfind(Protocol::DATA, 0) == 0) {
        size_t firstSep = command.find('|');
        size_t secondSep = command.find('|', firstSep + 1);
        size_t thirdSep = command.find('|', secondSep + 1);

        if (firstSep == string::npos || secondSep == string::npos || thirdSep == string::npos) {
            return Protocol::ERROR_INVALID_DATA;
        }

        string header = command.substr(0, firstSep);
        string aValues = command.substr(firstSep + 1, secondSep - firstSep - 1);
        string bValues = command.substr(secondSep + 1, thirdSep - secondSep - 1);
        string kValue = command.substr(thirdSep + 1);

        stringstream hs(header);
        string cmd;
        int tempDataSize;

        hs >> cmd >> tempDataSize;

        if (hs.fail() || tempDataSize <= 0) {
            return Protocol::ERROR_INVALID_DATA;
        }

        vector<int> tempA = parseArray(aValues);
        vector<int> tempB = parseArray(bValues);

        stringstream ks(kValue);
        int tempK;
        ks >> tempK;

        if (ks.fail()) {
            return Protocol::ERROR_INVALID_DATA;
        }

        if (tempA.size() != tempDataSize || tempB.size() != tempDataSize) {
            return Protocol::ERROR_INVALID_DATA;
        }

        {
            lock_guard<mutex> lock(session->dataMutex);
            session->dataSize = tempDataSize;
            session->A = tempA;
            session->B = tempB;
            session->k = tempK;
            session->C.clear();
        }

        session->isDone = false;
        session->isProcessing = false;

        cout << "Received DATA for client " << session->socket << '\n';
        cout << "Size: " << tempDataSize << '\n';
        cout << "k: " << tempK << '\n';

        return Protocol::OK;
    }

    if (command == Protocol::START) {
        if (session->isProcessing) {
            return Protocol::ALREADY_RUNNING;
        }

        {
            lock_guard<mutex> lock(session->dataMutex);

            if (session->A.empty() || session->B.empty() ||
                session->threadCount <= 0 || session->dataSize <= 0) {
                return Protocol::ERROR_NOT_READY;
            }
        }

        if (session->computationThread.joinable()) {
            session->computationThread.join();
        }

        session->isProcessing = true;
        session->isDone = false;

        cout << "Starting async computation for client " << session->socket << '\n';

        session->computationThread = thread(&Server::runComputation, this, session);

        return Protocol::OK;
    }

    if (command == Protocol::STATUS) {
        if (session->isProcessing) {
            return Protocol::IN_PROGRESS;
        }

        if (session->isDone) {
            return Protocol::DONE;
        }

        return Protocol::NOT_STARTED;
    }

    if (command == Protocol::RESULT) {
        if (session->isProcessing) {
            return Protocol::IN_PROGRESS;
        }

        if (!session->isDone) {
            return Protocol::ERROR_RESULT_NOT_READY;
        }

        return buildResultString(session);
    }

    return Protocol::ERROR_INVALID_COMMAND;
}

void Server::handleClient(shared_ptr<ClientSession> session) {
    while (true) {
        char buffer[4096] = {0};
        ssize_t receivedBytes = ::recv(session->socket, buffer, sizeof(buffer) - 1, 0);

        if (receivedBytes < 0) {
            perror("Receive failed");
            break;
        }

        if (receivedBytes == 0) {
            cout << "Client disconnected: " << session->socket << '\n';
            break;
        }

        buffer[receivedBytes] = '\0';
        string command(buffer);

        cout << "Client " << session->socket << " sent command: " << command << '\n';

        string response = processCommand(session, command);

        if (::send(session->socket, response.c_str(), response.size(), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (command == Protocol::EXIT) {
            cout << "Client session finished: " << session->socket << '\n';
            break;
        }
    }

    if (session->computationThread.joinable()) {
        session->computationThread.join();
    }

    ::close(session->socket);
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

    if (::listen(server_fd_, 10) < 0) {
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

        cout << "Client connected: " << clientSocket << '\n';

        auto session = make_shared<ClientSession>(clientSocket);

        thread clientThread(&Server::handleClient, this, session);
        clientThread.detach();
    }
}