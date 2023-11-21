#include "ti_server.h"
#include <argon2.h>
#include <log.h>
#include <nanoid.h>

#define PASSWORD_HASH_BYTES 64
#define PASSWORD_HASH_SALT "DIuL4dPTcL3q1a7EFOF9f"
#define PASSWORD_HASH_SALT_LEN 21

using namespace ti::server;
using namespace ti;

char *hash_password(std::string passcode) {
    char *encryption = (char *)calloc(PASSWORD_HASH_BYTES, sizeof(char));
    hash_argon2i(encryption, PASSWORD_HASH_BYTES, passcode.c_str(),
                 passcode.length(), PASSWORD_HASH_SALT, PASSWORD_HASH_SALT_LEN,
                 4, 1 << 16);
    return encryption;
}

ServerOrm::ServerOrm(const std::string &dbfile) : TiOrm(dbfile) {
    logD("[server orm] executing initializing SQL");
    exec_sql("CREATE TABLE IF NOT EXISTS \"password\"\n(\n"
             "    user_id varchar(21) primary key not null,\n"
             "    hash    blob                    not null,\n"
             "    FOREIGN KEY (user_id)\n"
             "        REFERENCES user (id)\n"
             "        ON DELETE CASCADE\n"
             ");\n"
             "CREATE TABLE IF NOT EXISTS \"token\"\n"
             "(\n"
             "    id         int primary key,\n"
             "    user_id    varchar(21) not null,\n"
             "    token      varchar(21) not null,\n"
             "    identifier varchar     not null,\n"
             "    FOREIGN KEY (user_id)\n"
             "        REFERENCES user (id)\n"
             "        ON DELETE CASCADE\n"
             ");\n"
             "CREATE TABLE IF NOT EXISTS \"contact\"\n"
             "(\n"
             "    id         integer primary key,\n"
             "    owner_id   varchar(21) not null,\n"
             "    contact_id varchar(21) not null\n"
             ");\n");
}
void ServerOrm::pull() {
    orm::TiOrm::pull();
    contacts.clear();
    auto t = prepare("SELECT * FROM \"contact\"");
    for (auto e : *t) {
        contacts.emplace_back(get_user(e.get_text(0)),
                              get_entity(e.get_text(1)));
    }
    delete t;

    tokens.clear();
    t = prepare("SELECT user_id, token FROM \"token\"");
    for (auto e : *t) {
        tokens.emplace_back(get_user(e.get_text(0)), e.get_text(1));
    }
    delete t;
}
std::vector<Entity *> ServerOrm::get_contacts(const User &owner) const {
    std::vector<Entity *> ev;
    for (auto e : contacts) {
        if (*e.first == owner) {
            ev.push_back(e.second);
        }
    }
    return ev;
}
bool ServerOrm::check_password(const std::string &user_id,
                               const std::string &passcode) const {
    auto t = prepare("SELECT hash FROM password WHERE user_id = ?");
    t->bind_text(0, user_id);
    auto beg = t->begin();
    if (beg == t->end()) {
        return false;
    }
    const void *buf;
    auto n = (*beg).get_blob(0, &buf);
    if (n != PASSWORD_HASH_BYTES) {
        throw std::runtime_error("corrupt password database");
    }
    auto hashed = hash_password(passcode);
    for (int i = 0; i < PASSWORD_HASH_BYTES; ++i) {
        if (hashed[i] != ((char *)buf)[i]) {
            return false;
        }
    }
    delete t;
    return true;
}
User *ServerOrm::check_token(const std::string &token) const {
    for (const auto &t : tokens) {
        if (t.second == token) {
            return t.first;
        }
    }
    return nullptr;
}
void ServerOrm::add_token(User *owner, const std::string &token) {
    tokens.emplace_back(owner, token);
    auto t = prepare("INSERT INTO \"token\"(user_id, token)  VALUES (?, ?)");
    t->bind_text(0, owner->get_id());
    t->bind_text(1, token);
    t->begin();
    delete t;
}
bool ServerOrm::invalidate_token(int token_id, User *owner) {
    if (owner == nullptr) {
        auto t = prepare("DELETE FROM token WHERE id = ?");
        t->bind_int(0, token_id);
        t->begin();
        delete t;
    } else {
        auto t = prepare("DELETE FROM token WHERE id = ? AND user_id = ?");
        t->bind_int(0, token_id);
        t->bind_text(1, owner->get_id());
        t->begin();
        delete t;
    }
    return get_changes() > 0;
}
bool ServerOrm::invalidate_token(const std::string &token, User *owner) {
    int i;
    bool found = false;
    for (i = 0; i < tokens.size(); i++) {
        if (owner == nullptr ||
            *tokens[i].first == *owner && tokens[i].second == token) {
            found = true;
            break;
        }
    }
    if (found) {
        tokens.erase(tokens.begin() + i);
        auto t = prepare("DELETE FROM token WHERE token = ?");
        t->bind_text(0, token);
        delete t;
    }
    return found;
}

void ServerOrm::add_user(ti::User *user, const std::string &passcode) {
    add_entity(user);
    auto t = prepare("INSERT INTO password VALUES (?, ?)");
    t->bind_text(0, user->get_id());
    auto hash = hash_password(passcode);
    t->bind_blob(1, (void *)hash, PASSWORD_HASH_BYTES);
    t->begin();
    delete t;
    delete hash;
}

TiServer::TiServer(std::string addr, short port, const std::string &dbfile)
    : Server(std::move(addr), port), db(dbfile) {
    db.pull();
}
TiServer::~TiServer() = default;
Client *TiServer::on_connect(sockaddr_in addr) { return new TiClient(db); }

std::vector<std::string> read_message_body(const char *data, size_t len) {
    int i = 0, j = 0;
    std::vector<std::string> heap;
    while (i < len) {
        if (data[i] == '\0') {
            std::string substr(data + j, i - j);
            heap.push_back(substr);
            j = i + 1;
        }
        i++;
    }
    if (j != i) {
        std::string substr(data + j, len - j);
        heap.push_back(substr);
    }
    return heap;
}

TiClient::TiClient(ServerOrm &db)
    : db(db), user(nullptr), token(), id(nanoid::generate()) {}
TiClient::~TiClient() = default;
void TiClient::on_connect(sockaddr_in addr) {
    logD("[client %s] connected to %s", id.c_str(), inet_ntoa(addr.sin_addr));
}
void TiClient::on_message(ti::RequestCode req, char *data, size_t len) {
    auto body = read_message_body(data, len);
    switch (req) {
    case LOGIN:
        logD("[client %s] login(%s, %s)", id.c_str(), body[0].c_str(), body[1].c_str());
        user_login(body[0], body[1]);
        break;
    case LOGOUT:
        logD("[client %s] logout(%s)", id.c_str(), body[0].c_str());
        logout(body[0]);
        break;
    case REGISTER:
        logD("[client %s] register(%s, %s)", id.c_str(), body[0].c_str(), body[1].c_str());
        user_register(body[0], body[1]);
        break;
    case DETERMINE:
        logD("[client %s] determine(%s, %s)", id.c_str(), body[0].c_str(), body[1].c_str());
        determine(body[0], std::stoi(body[1]));
        break;
    case RECONNECT:
        logD("[client %s] reconnect(%s)", id.c_str(), body[0].c_str());
        reconnect(body[0]);
        break;
    }
}

void TiClient::user_login(const std::string &user_id,
                          const std::string &password) {
    if (db.check_password(user_id, password)) {
        auto res_buf = (char *)calloc(21, sizeof(char));
        token = nanoid::generate();
        user = db.get_user(user_id);
        strcpy(res_buf, token.c_str());
        send(ResponseCode::OK, res_buf, sizeof res_buf);
        db.add_token(user, token);
        logD("[client %s] logged in as %s", id.c_str(), user_id.c_str());
    } else {
        send(ResponseCode::NOT_FOUND, nullptr, 0);
    }
}

void TiClient::reconnect(const std::string &old_token) {
    user = db.check_token(old_token);
    if (user == nullptr) {
        send(ti::ResponseCode::NOT_FOUND);
    } else {
        token = old_token;
        send(ti::ResponseCode::OK);
    }
}

void TiClient::determine(const std::string &curr_token, int token_id) {
    if (curr_token != token || user == nullptr) {
        send(ResponseCode::TOKEN_EXPIRED);
    } else if (db.invalidate_token(token_id, user)) {
        send(ResponseCode::OK);
    } else {
        send(ResponseCode::NOT_FOUND);
    }
}

void TiClient::logout(const std::string &old_token) {
    if (token.empty() || user == nullptr) {
        send(ResponseCode::TOKEN_EXPIRED);
    } else if (db.invalidate_token(token, user)) {
        send(ResponseCode::OK);
    } else {
        send(ResponseCode::NOT_FOUND);
    }
}

void TiClient::user_register(const std::string &user_name,
                             const std::string &passcode) {
    auto invalid = std::find_if(user_name.begin(), user_name.end(),
                                [&](const char c) { return c < 32; });
    if (invalid != user_name.end()) {
        send(ResponseCode::BAD_REQUEST);
    } else {
        auto user_id = nanoid::generate();
        db.add_user(new User(user_id, user_name, {}), passcode);
        send(ResponseCode::OK);
    }
}

void TiClient::on_disconnect() { logD("%s disconnected", id.c_str()); }
