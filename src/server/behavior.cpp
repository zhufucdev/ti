#include <server/behavior.hpp>

using namespace ti::server;

TiServer::TiServer(std::string addr, short port, std::string dbfile)
    : Server(std::move(addr), port) {
    int n = sqlite3_open(dbfile.c_str(), &dbhandle);
    if (n != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
}
TiServer::~TiServer() { sqlite3_close(dbhandle); }
Client *TiServer::on_connect(sockaddr_in addr) {
    return new TiClient(dbhandle);
}

TiClient::TiClient(sqlite3 *dbhandle)
    : dbhandle(dbhandle), id(nanoid::generate()) {}
TiClient::~TiClient() {}
void TiClient::on_connect(sockaddr_in addr) {
    std::cout << "connected to " << inet_ntoa(addr.sin_addr) << " as " << id
              << std::endl;
}
void TiClient::on_message(char *content, int len) { send(content, len); }
void TiClient::on_disconnect() { std::cout << id << " disconnected" << std::endl; }
