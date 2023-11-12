#include <client/protocol.hpp>

using namespace ti::client;

Client::Client(std::string addr, short port)
    : addr(std::move(addr)), port(port) {}
Client::~Client() { ::closesocketfd(socketfd); }
void Client::start() {
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in serveraddr;
    std::memset(&serveraddr, 0, sizeof serveraddr);
    serveraddr.sin_addr.s_addr = inet_addr(addr.c_str());
    if (::connect(socketfd, (struct sockaddr *)&serveraddr, sizeof serveraddr) <
        0) {
        throw std::runtime_error("Failed to start");
    }
    std::thread([&] {

    });
}
void Client::stop() {
    ::closesocketfd(socketfd);
}
void Client::send(const void *data, size_t len) {
    ::send(socketfd, data, len, 0);
}
