#include "server.h"
#include <thread>
#include <helper.h>

using namespace ti::server;

void Client::initialize(SendFn fn) { sendfn = std::move(fn); }
void Client::send(ti::ResponseCode res, void *content, size_t len) const {
    sendfn(res, content, len);
}
void Client::send(ti::ResponseCode res) const { sendfn(res, nullptr, 0); }

Server::Server(std::string addr, short port)
    : addr(std::move(addr)), port(port), running(false) {}

Server::~Server() {
    if (running) {
        closesocketfd(socketfd);
    }
}

void Server::start() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("failed to initialize winsock");
    }
    socketfd = socket(PF_INET, SOCK_STREAM, 0);
    if (socketfd == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("failed to create socket");
    }
    sockaddr_in servaddr;
    ZeroMemory(&servaddr, sizeof(servaddr));
#else
    socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
    if (socketfd < 0) {
        throw std::runtime_error("failed to create socket");
    }
    sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof servaddr);
#endif

    servaddr.sin_family = PF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(PF_INET, addr.c_str(), &servaddr.sin_addr) <= 0) {
        throw std::runtime_error("invalid address");
    }
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof servaddr) < 0) {
        throw std::runtime_error("failed to bind");
    }
    if (listen(socketfd, 10) < 0) {
        throw std::runtime_error("failed to listen");
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
        handleconn(clientaddr, clientfd);
    }

    closesocketfd(socketfd);
    running = false;
}

void Server::stop() {
    if (!running) {
        throw std::runtime_error("the server is currently not running");
    }
    running = false;
    closesocketfd(socketfd);
}

void Server::handleconn(sockaddr_in addr, SocketFd clientfd) {
    std::thread([&] {
        auto *handler = this->on_connect(addr);
        handler->initialize([&](ResponseCode res, void *content, int len) {
            ti::server::Server::send(clientfd, res, content, len);
        });
        handler->on_connect(addr);

        char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char)),
             *treq = (char *)calloc(1, sizeof(char)), *buff = nullptr;
        size_t msize;

        while (true) {
            size_t n = recv(clientfd, treq, 1, 0);
            if (n <= 0) {
                break;
            }
            n = recv(clientfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
            if (n <= 0) {
                break;
            }
            msize = ti::helper::read_len_header(tsize);
            buff = (char *)malloc(msize);
            n = recv(clientfd, buff, msize, 0);
            if (n <= 0) {
                break;
            }
            handler->on_message((RequestCode)*treq, buff, msize);
        }

        closesocketfd(clientfd);
        handler->on_disconnect();
        delete handler;
        delete tsize;
        delete buff;
    }).detach();
}

void Server::send(SocketFd clientfd, ResponseCode res, void *data, size_t len) {
    auto *tres = (char *)calloc(1, sizeof(char));
    tres[0] = res;
    compat::socket::send(clientfd, tres, sizeof(char), 0);
    char *tsize = ti::helper::write_len_header(len);
    compat::socket::send(clientfd, tsize, sizeof(char) * BYTES_LEN_HEADER, 0);
    if (len > 0) {
        compat::socket::send(clientfd, data, len, 0);
    }
    delete tsize;
    delete tres;
}

std::string Server::get_addr() const { return addr; }

short Server::get_port() const { return port; }

bool Server::is_running() const { return running; }
