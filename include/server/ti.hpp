#include "../ti.hpp"
#include <iostream>
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
#define closesocketfd(A) closesocket(A); WSACleanup()
#define SocketFd SOCKET
#else
#define closesocketfd(A) close(A)
#define SocketFd int
#endif

namespace ti {
class TiServer {
    bool running;
    std::string *addr;
    short port;

  public:
    TiServer(std::string *addr, short port) : addr(addr), port(port){};
    void start();
    void handleconn(SocketFd clientfd);
};
} // namespace ti

void ti::TiServer::start() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("Failed to initialize winsock");
    }
    SocketFd socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }
    sockaddr_in servaddr;
    ZeroMemory(&servaddr, sizeof(servaddr)); 
#else
    SocketFd socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));
#endif

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = port;
    if (inet_pton(AF_INET, addr->c_str(), &servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        throw std::runtime_error("Failed to bind");
    }
    if (listen(socketfd, 10) < 0) {
        throw std::runtime_error("Failed to listen");
    }

    while (running) {
        sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        SocketFd clientfd =
            accept(socketfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (clientfd < 0) {
            throw std::runtime_error("Failed to accept incoming connection");
        }
        std::thread(&ti::TiServer::handleconn, this, clientfd).detach();
    }
}

void ti::TiServer::handleconn(SocketFd clientfd) {
    char buf[1024];
    while (true)
    {
        std::memset(buf, 0, sizeof(buf));
        int n = recv(clientfd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        send(clientfd, buf, n, 0);
    }
    
    closesocketfd(clientfd);
}