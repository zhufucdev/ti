#include "ti_client.h"
#include "log.h"

using namespace ti::client;
using namespace ti;

TiClient::TiClient(std::string addr, short port, const std::string &dbfile)
    : Client(std::move(addr), port), ti::orm::TiOrm(dbfile),
      state(TiClientState::OFFLINE), userid(), token() {}
TiClient::~TiClient() { sqlite3_close(dbhandle); }
void TiClient::on_message(char *data, size_t len) {}
void TiClient::on_connect(sockaddr_in serveraddr) {
    logD("[client] connected to %s", inet_ntoa(serveraddr.sin_addr));
}
void TiClient::on_close() { logD("[client] closed"); }

#define panic_unknown_res(method, code)                                        \
    logD("[client] %s failed with code %d", method, code);                     \
    throw std::runtime_error("unknown response code")
TiClientState TiClient::get_state() { return state; }
bool TiClient::user_login(const std::string &user_id,
                          const std::string &password) {
    auto res = Client::send(RequestCode::LOGIN, user_id, password);
    switch (res.code) {
    case ResponseCode::NOT_FOUND:
        return false;
    case ResponseCode::OK:
        state = READY;
        token = std::string(res.buff, res.len);
        return true;
    default:
        logD("[client] login failed with code %d", res.code);
        panic_unknown_res("login", res.code);
    }
}
bool TiClient::user_reg(const std::string &name, const std::string &password) {
    auto res = Client::send(RequestCode::REGISTER, name, password);
    switch (res.code) {
    case ResponseCode::BAD_REQUEST:
        return false;
    case ResponseCode::OK:
        return true;
    default:
        panic_unknown_res("registry", res.code);
    }
}
bool TiClient::user_logout() {
    if (state < READY) {
        return false;
    }
    auto res = Client::send(RequestCode::LOGOUT, token);
    switch (res.code) {
    case ResponseCode::TOKEN_EXPIRED:
    case ResponseCode::NOT_FOUND:
        return false;
    case ResponseCode::OK:
        return true;
    default:
        panic_unknown_res("logout", res.code);
    }
}
std::vector<User *> TiClient::get_current_user() const {}
std::vector<Entity *> TiClient::get_contacts() const {}
void TiClient::send(ti::Message *message) {}
