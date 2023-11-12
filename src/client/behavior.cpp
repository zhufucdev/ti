#include <client/behavior.hpp>
#include <log.hpp>

using namespace ti::client;
using namespace ti;

TiClient::TiClient(std::string addr, short port, std::string dbfile)
    : Client(std::move(addr), port) {
    int n = sqlite3_open(dbfile.c_str(), &dbhandle);
    if (n != SQLITE_OK) {
        throw std::runtime_error("Failed to load database");
    }
}
TiClient::~TiClient() { sqlite3_close(dbhandle); }
void TiClient::on_message(char *data, size_t len) {
}
void TiClient::on_connect(sockaddr_in serveraddr) {
    logD("Connected to %s", inet_ntoa(serveraddr.sin_addr));
}
void TiClient::on_close() {

}
VerificationResult TiClient::user_login(std::string name,
                                        std::string password) {

}
bool TiClient::user_reg(std::string name, std::string password) {

}
bool TiClient::user_logout() {

}
std::vector<User *> TiClient::get_current_user() {

}
std::vector<Entity *> TiClient::get_contacts() {

}
Entity *TiClient::get_entity(std::string id) {

}
void TiClient::send(ti::Message *message) {

}

