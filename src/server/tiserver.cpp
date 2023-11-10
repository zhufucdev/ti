#include <server/ti.hpp>

#include <cstring>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define closesocketfd(A)                                                       \
    closesocket(A);                                                            \
    WSACleanup()
#else
#define closesocketfd(A) close(A)
#endif

void ti::TiServer::start() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("Failed to initialize winsock");
    }
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }
    sockaddr_in servaddr;
    ZeroMemory(&servaddr, sizeof(servaddr));
#else
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (socketfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));
#endif

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        throw std::runtime_error("Failed to bind");
    }
    if (listen(socketfd, 10) < 0) {
        throw std::runtime_error("Failed to listen");
    }

    running = true;
    while (running) {
        sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        SocketFd clientfd =
            accept(socketfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (clientfd < 0) {
            continue;
        }
        std::thread(&ti::TiServer::handleconn, this, clientfd).detach();
    }

    closesocketfd(socketfd);
    running = false;
}

void ti::TiServer::handleconn(SocketFd clientfd) {
    char buf[1024] = {0};
    while (true) {
        int n = recv(clientfd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        send(clientfd, buf, n, 0);
    }

    closesocketfd(clientfd);
}

void ti::TiServer::stop() {
    if (!running) {
        throw std::runtime_error("The server is currently not running");
    }
    running = false;
    closesocketfd(socketfd);
}

std::string ti::TiServer::get_addr() {
    return addr;
}

short ti::TiServer::get_port() {
    return port;
}