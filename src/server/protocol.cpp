#include <server/protocol.hpp>

using namespace ti::server;

void Client::initialize(SendFn fn) { sendfn = std::move(fn); }
void Client::send(char *content, size_t len) const { sendfn(content, len); }

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
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("failed to create socket");
    }
    sockaddr_in servaddr;
    ZeroMemory(&servaddr, sizeof(servaddr));
#else
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (socketfd < 0) {
        throw std::runtime_error("failed to create socket");
    }
    sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof servaddr);
#endif

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0) {
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

void Server::handleconn(sockaddr_in addr, int clientfd) {
    std::thread([&] {
        auto *handler = this->on_connect(addr);
        handler->initialize([&](char *content, int len) {
            this->send(clientfd, content, len);
        });
        handler->on_connect(addr);

        char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char)),
             *buff = nullptr;
        size_t msize;

        while (true) {
            ssize_t n = recv(clientfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
            if (n <= 0) {
                break;
            }
            msize = read_len_header(tsize);
            buff = (char *)malloc(msize);
            n = recv(clientfd, buff, msize, 0);
            if (n <= 0) {
                break;
            }
            handler->on_message(buff, msize);
        }

        closesocketfd(clientfd);
        handler->on_disconnect();
        delete handler;
        delete tsize;
        delete buff;
    }).detach();
}

void Server::send(int clientfd, char *data, size_t len) {
    char *tsize = write_len_header(len);
    ::send(clientfd, tsize, sizeof(char) * BYTES_LEN_HEADER, 0);
    ::send(clientfd, data, len, 0);
}

std::string Server::get_addr() const { return addr; }

short Server::get_port() const { return port; }

bool Server::is_running() const { return running; }
