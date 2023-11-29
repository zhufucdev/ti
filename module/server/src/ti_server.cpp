#include "ti_server.h"
#include <argon2.h>
#include <log.h>
#include <nanoid.h>
#include <ranges>

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
std::vector<Entity *> ServerOrm::get_contacts(const User *owner) const {
    std::vector<Entity *> ev;
    for (auto e : contacts) {
        if (*e.first == *owner) {
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

std::vector<std::string> read_message_body(const char *data, size_t len,
                                           char separator = '\0') {
    int i = 0, j = 0;
    std::vector<std::string> heap;
    while (i < len) {
        if (data[i] == separator) {
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
        logD("[client %s] login(%s, %s)", id.c_str(), body[0].c_str(),
             body[1].c_str());
        user_login(body[0], body[1]);
        break;
    case LOGOUT:
        logD("[client %s] logout(%s)", id.c_str(), body[0].c_str());
        logout(body[0]);
        break;
    case REGISTER:
        logD("[client %s] register(%s, %s)", id.c_str(), body[0].c_str(),
             body[1].c_str());
        user_register(body[0], body[1]);
        break;
    case SYNC:
        logD("[client %s] sync(%s, %s)", id.c_str(), body[0].c_str(),
             body[1].c_str());
        sync(body[0], body[1]);
        break;
    case DELETE_USER:
        logD("[client %s] delete_user(%s)", id.c_str(), body[0].c_str());
        user_delete(body[0]);
        break;
    case DETERMINE:
        logD("[client %s] determine(%s, %s)", id.c_str(), body[0].c_str(),
             body[1].c_str());
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
        token = nanoid::generate();
        user = db.get_user(user_id);
        send(ResponseCode::OK, (void *)token.c_str(), token.length());
        db.add_token(user, token);
        logD("[client %s] logged in as %s", id.c_str(), user_id.c_str());
    } else {
        send(ResponseCode::NOT_FOUND);
    }
}

void TiClient::user_delete(const std::string &curr_token) {
    if (token != curr_token || user == nullptr) {
        send(ResponseCode::TOKEN_EXPIRED);
    } else {
        try {
            db.delete_entity(user);
            send(ResponseCode::OK);
        } catch (std::runtime_error &e) {
            send(ResponseCode::NOT_FOUND);
        }
    }
}

void TiClient::reconnect(const std::string &old_token) {
    user = db.check_token(old_token);
    if (user == nullptr) {
        send(ti::ResponseCode::NOT_FOUND);
    } else {
        token = old_token;
        auto res = user->get_id();
        send(ti::ResponseCode::OK, (void *)res.c_str(), res.length());
    }
}

size_t memappend(char **src, size_t src_count, size_t *src_len, char *dest) {
    size_t sum = 0;
    for (size_t i = 0; i < src_count; ++i) {
        std::memcpy(dest + sum, src, src_len[i]);
        sum += src_len[0];
    }
    return sum;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
size_t write_sync_response(char **buf, std::vector<Entity *> *entities,
                        std::vector<Frame *> *frames,
                        std::vector<Message *> *messages) {
    size_t frames_mask = frames != nullptr ? 1 : 0,
           messages_mask = messages != nullptr ? 1 : 0,
           entities_mask = entities != nullptr ? 1 : 0;
    char *frame_bufs[frames_mask * frames->size()],
        *msg_bufs[messages_mask * messages->size()],
        *entity_bufs[entities_mask * entities->size()];
    size_t frame_lens[frames_mask * frames->size()],
        msg_lens[messages_mask * messages->size()],
        entity_lens[entities_mask * entities->size()];
    size_t sending_len = 0;
    for (int i = 0; i < entities_mask * entities->size(); i++) {
        entity_lens[i] = (*entities)[i]->serialize(&entity_bufs[i]);
        sending_len += entity_lens[i];
    }
    for (int i = 0; i < frames_mask * frames->size(); i++) {
        frame_lens[i] = (*frames)[i]->serialize(&frame_bufs[i]);
        sending_len += frame_lens[i];
    }
    for (int i = 0; i < messages_mask * messages->size(); i++) {
        msg_lens[i] = (*messages)[i]->serialize(&msg_bufs[i]);
        sending_len += msg_lens[i];
    }

    char *sending_buf = (char *)calloc(
        sending_len +
            (entities_mask + frames_mask + messages_mask) * BYTES_LEN_HEADER,
        sizeof(char));
    sending_len = 0;
    if (entities_mask) {
        auto len_header = write_len_header(entities->size());
        std::memcpy(sending_buf, len_header, BYTES_LEN_HEADER);
        sending_len += BYTES_LEN_HEADER;
        sending_len +=
            memappend(entity_bufs, entities->size(), entity_lens, sending_buf);
        delete len_header;
    }
    if (frames_mask) {
        auto len_header = write_len_header(frames->size());
        std::memcpy(sending_buf + sending_len, len_header, BYTES_LEN_HEADER);
        sending_len += memappend(frame_bufs, frames->size(), frame_lens,
                                 sending_buf + sending_len);
        delete len_header;
    }
    if (messages_mask) {
        auto len_header = write_len_header(messages->size());
        std::memcpy(sending_buf + sending_len, len_header, BYTES_LEN_HEADER);
        memappend(msg_bufs, messages->size(), msg_lens,
                  sending_buf + sending_len);
        delete len_header;
    }
    *buf = sending_buf;
    return sending_len;
}
#pragma clang diagnostic pop

void TiClient::sync(const std::string &curr_token,
                    const std::string &selector) {
    if (curr_token != token || user == nullptr) {
        send(ResponseCode::TOKEN_EXPIRED);
    } else {
        auto paths =
            read_message_body(selector.c_str(), selector.length(), '/');
        if (paths[0] == "*") {
            auto contacts = db.get_contacts(user);
            std::vector<std::string> group_ids;
            std::transform(
                std::find_if(contacts.begin(), contacts.end(),
                             [&](Entity *ctc) {
                                 return (dynamic_cast<Group *>(ctc) != nullptr);
                             }),
                contacts.end(), std::back_inserter(group_ids),
                [&](Entity *e) { return e->get_id(); });
            auto all_messages = db.get_messages();
            auto messages = std::vector<Message *>(
                std::find_if(all_messages.begin(), all_messages.end(),
                             [&](Message *msg) {
                                 if (msg->get_receiver() == user) {
                                     return true;
                                 }
                                 return std::find(
                                            group_ids.begin(), group_ids.end(),
                                            msg->get_receiver()->get_id()) !=
                                        group_ids.end();
                             }),
                all_messages.end());
            std::vector<Frame *> frames;
            for (auto msg : messages) {
                std::copy_if(msg->get_frames().begin(), msg->get_frames().end(),
                             std::back_inserter(frames), [&](Frame *frame) {
                                 return std::find(frames.begin(), frames.end(),
                                                  frame) == frames.end();
                             });
            }
            char *buf;
            size_t len =
                write_sync_response(&buf, &contacts, &frames, &messages);
            send(ResponseCode::OK, buf, len);
        } else if (Entity *entity = db.get_entity(paths[0])) {
            if (paths.size() < 2 || paths[1] == "*") {
                char *buf;
                size_t len = entity->serialize(&buf);
                send(ResponseCode::OK, buf, len);
            } else if (paths[1] == "id") {
                send(ResponseCode::OK, (void *)paths[0].c_str(),
                     paths[0].length());
            } else if (paths[1] == "name") {
                std::string name;
                if (auto *u = dynamic_cast<User *>(entity)) {
                    name = u->get_name();
                } else if (auto *g = dynamic_cast<Group *>(entity)) {
                    name = g->get_name();
                } else {
                    send(ResponseCode::BAD_REQUEST);
                    return;
                }
                send(ResponseCode::OK, (void *)name.c_str(), name.length());
            } else if (paths[1] == "bio") {
                if (auto *u = dynamic_cast<User *>(entity)) {
                    send(ResponseCode::OK, (void *)u->get_bio().c_str(),
                         u->get_bio().length());
                } else {
                    send(ResponseCode::BAD_REQUEST);
                }
            } else if (paths[1] == "members") {
            }
        } else if (Message *message = db.get_message(paths[0])) {

        } else {
            send(ResponseCode::NOT_FOUND);
        }
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

void TiClient::logout(const std::string &curr_token) {
    if (token != curr_token || user == nullptr) {
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
        db.add_user(new User(user_id, user_name, {}, 0), passcode);
        send(ResponseCode::OK, (void *)user_id.c_str(), user_id.length());
    }
}

void TiClient::on_disconnect() { logD("%s disconnected", id.c_str()); }
