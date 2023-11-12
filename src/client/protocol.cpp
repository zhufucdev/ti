#include <client/ti.hpp>

using namespace ti::client;

Client::Client(std::string addr, short port)
    : addr(std::move(addr)), port(port) {}
Client::~Client() { ::closesocketfd(socketfd); }
void Client::connect() {
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in serveraddr;
    std::memset(&serveraddr, 0, sizeof serveraddr);
    serveraddr.sin_addr.s_addr = inet_addr(addr.c_str());
    if (::connect(socketfd, (struct sockaddr *)&serveraddr, sizeof serveraddr) <
        0) {
        throw std::runtime_error("Failed to connect");
    }
}
void Client::close() {
    ::closesocketfd(socketfd);
}
void Client::send(const void *data, size_t len) {
    ::send(socketfd, data, len, 0);
}

TiClient::TiClient(std::string addr, short port, std::string dbfile)
    : Client(addr, port) {
    int n = sqlite3_open(dbfile.c_str(), &dbhandle);
    if (n != SQLITE_OK) {
        throw std::runtime_error("Failed to load database");
    }
}
TiClient::~TiClient() { sqlite3_close(dbhandle); }
VerificationResult TiClient::user_login(std::string name,
                                        std::string password) {

}