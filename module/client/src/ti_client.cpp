#include <log.h>
#include <thread>
#include <ti_client.h>

using namespace ti::client;
using namespace ti;

TiClient::TiClient(std::string addr, short port, const std::string &dbfile)
    : Client(std::move(addr), port), ti::orm::TiOrm(dbfile),
      state(TiClientState::OFFLINE), userid(), token() {
    logD("[client] executing initializing SQL");
    exec_sql(R"(CREATE TABLE IF NOT EXISTS "session"
(
    user_id    varchar(21) primary key,
    token      varchar(21) not null,
    last_login datetime,
    foreign key (user_id)
        references user (id)
))");
}
TiClient::~TiClient() = default;
void TiClient::on_message(char *data, size_t len) {}
void TiClient::on_connect(sockaddr_in serveraddr) {
    logD("[client] connected to %s", inet_ntoa(serveraddr.sin_addr));
    state = LOGGED_OUT;
}
void TiClient::on_close() {
    logD("[client] closed");
    state = OFFLINE;
}

#define panic_unknown_res(method, code)                                        \
    logD("[client] %s failed with code %d", method, code);                     \
    throw std::runtime_error("unknown response code")

bool TiClient::try_reconnect() {
    auto t = prepare(
        R"(SELECT "token" FROM "session" ORDER BY "last_login" DESC LIMIT 1)");
    for (auto row : *t) {
        token = row.get_text(0);
    }
    return reconnect();
}
bool TiClient::reconnect(const std::string &new_token) {
    auto t = new_token.empty() ? token : new_token;
    if (t.empty()) {
        throw std::runtime_error("no token available");
    }
    auto res = Client::send(RequestCode::RECONNECT, t);
    switch (res.code) {
    case ResponseCode::OK:
        break;
    case ResponseCode::NOT_FOUND:
        return false;
    default:
        panic_unknown_res("reconnect", res.code);
    }
    userid = res.buff;
    if (!sync()) {
        logD("[client] warning: sync failed at reconnection");
    }
    return true;
}

bool TiClient::sync() {

    return true;
}

bool TiClient::user_login(const std::string &user_id,
                          const std::string &password) {
    panic_if_not(LOGGED_OUT);
    auto res = Client::send(RequestCode::LOGIN, user_id, password);
    switch (res.code) {
    case ResponseCode::NOT_FOUND:
        return false;
    case ResponseCode::OK:
        break;
    default:
        logD("[client] login failed with code %d", res.code);
        panic_unknown_res("login", res.code);
    }

    state = READY;
    token = std::string(res.buff, res.len);
    delete res.buff;
    auto t = prepare(R"(INSERT INTO "session" VALUES (?, ?, datetime('now')))");
    t->bind_text(0, user_id);
    t->bind_text(1, token);
    t->begin();

    return true;
}

std::string TiClient::user_reg(const std::string &name,
                               const std::string &password) {
    panic_if_not(LOGGED_OUT);
    auto res = Client::send(RequestCode::REGISTER, name, password);
    switch (res.code) {
    case ResponseCode::BAD_REQUEST:
        return std::string{};
    case ResponseCode::OK:
        return {res.buff, (std::string::size_type)res.len};
    default:
        panic_unknown_res("registry", res.code);
    }
}

bool TiClient::user_logout() {
    panic_if_not(READY);
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

bool TiClient::user_delete() {
    panic_if_not(READY);
    auto res = Client::send(RequestCode::DELETE_USER, token);
    switch (res.code) {
    case ResponseCode::OK:
        return true;
    case ResponseCode::NOT_FOUND:
    case ResponseCode::TOKEN_EXPIRED:
        return false;
    default:
        panic_unknown_res("user_delete", res.code);
    }
}
TiClientState TiClient::get_state() { return state; }
User *TiClient::get_current_user() const { return get_user(userid); }
std::vector<Entity *> TiClient::get_contacts() const {}
void TiClient::send(ti::Message *message) {}

void TiClient::panic_if_not(ti::client::TiClientState target) {
    if (state < target) {
        throw std::runtime_error("illegal client state");
    }
}
