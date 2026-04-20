#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 4096

string readFile(const string& filePath) {
    ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string getRequestedPath(const string& request) {
    istringstream requestStream(request);
    string method;
    string path;
    string version;

    requestStream >> method >> path >> version;

    return path;
}

int main() {
    int serverSocket;
    sockaddr_in serverAddr{};

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Socket creation failed\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "setsockopt failed\n";
        close(serverSocket);
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (::bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        cerr << "Bind failed\n";
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        cerr << "Listen failed\n";
        close(serverSocket);
        return 1;
    }

    cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            cerr << "Accept failed\n";
            continue;
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            close(clientSocket);
            continue;
        }

        string request(buffer);

        cout << "\n--- HTTP REQUEST START ---\n";
        cout << request << '\n';
        cout << "--- HTTP REQUEST END ---\n\n";

        string path = getRequestedPath(request);
        string filePath;

        if (path == "/") {
            filePath = "site/index.html";
        } else {
            filePath = "site" + path;
        }

        string body = readFile(filePath);
        string httpResponse;

        if (!body.empty()) {
            httpResponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Content-Length: " + to_string(body.size()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                body;
        } else {
            string notFoundBody =
                "<!DOCTYPE html>"
                "<html>"
                "<head><title>404 Not Found</title></head>"
                "<body>"
                "<h1>404 Not Found</h1>"
                "<p>The requested page does not exist.</p>"
                "</body>"
                "</html>";

            httpResponse =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Content-Length: " + to_string(notFoundBody.size()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                notFoundBody;
        }

        send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}