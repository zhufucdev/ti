#include <client/behavior.hpp>

using namespace ti::client;

TiClient::TiClient(std::string addr, short port, std::string dbfile)
    : Client(std::move(addr), port) {
    int n = sqlite3_open(dbfile.c_str(), &dbhandle);
    if (n != SQLITE_OK) {
        throw std::runtime_error("Failed to load database");
    }
}
TiClient::~TiClient() { sqlite3_close(dbhandle); }
VerificationResult TiClient::user_login(std::string name,
                                        std::string password) {

}
