#include <server/protocol.hpp>

using namespace ti::server;

void Client::initialize(SendFn fn) { sendfn = std::move(fn); }
void Client::send(char *content, int len) const { sendfn(content, len); }

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
    std::memset(&servaddr, 0, sizeof servaddr);
#endif

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof servaddr) < 0) {
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
        handleconn(clientaddr, clientfd);
    }

    closesocketfd(socketfd);
    running = false;
}

void Server::stop() {
    if (!running) {
        throw std::runtime_error("The server is currently not running");
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

        char t;
        char *buf = (char *)calloc(BUFFER_SIZE, sizeof(char));
        int cap = BUFFER_SIZE, top = 0, n;

        while (true) {
            while ((n = recv(clientfd, &t, 1, 0)) > 0) {
                buf[top++] = t;
                if (top >= cap) {
                    cap += BUFFER_SIZE;
                    buf = (char *)realloc(buf, sizeof(char) * cap);
                    if (!buf) {
                        throw std::overflow_error("realloc failed");
                    }
                }

                if (t == '\0') {
                    // end of message
                    break;
                }
            }
            if (n <= 0) {
                break;
            }

            handler->on_message(buf, top);
            free(buf);
            buf = (char *)calloc(BUFFER_SIZE, sizeof(char));
            top = 0;
        }

        closesocketfd(clientfd);
        handler->on_disconnect();
        delete handler;
        free(buf);
    }).detach();
}

void Server::send(int clientfd, char *data, int len) {
    ::send(clientfd, data, len, 0);
}

std::string Server::get_addr() { return addr; }

short Server::get_port() { return port; }
