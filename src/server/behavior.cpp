#include <log.hpp>
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

enum RequestType {
    LOGIN = 0x00,
};

TiClient::TiClient(sqlite3 *dbhandle)
    : dbhandle(dbhandle), id(nanoid::generate()) {}
TiClient::~TiClient() = default;
void TiClient::on_connect(sockaddr_in addr) {
    logD("Connected to %s as %s", inet_ntoa(addr.sin_addr), id.c_str());
}
void TiClient::on_message(char *content, size_t len) {
    auto req_t = (RequestType)content[0];
    switch (req_t) {
    case LOGIN:

        break;
    }
    send(content, len);
}
void TiClient::on_disconnect() { logD("%s disconnected", id.c_str()); }
