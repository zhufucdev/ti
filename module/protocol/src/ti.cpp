#include "ti.h"
#include "log.h"
#include <algorithm>
#include <ctime>
#include <numeric>
#include <sstream>
#include <timecompat.h>

using namespace ti;
using namespace orm;

std::vector<std::string> ti::read_message_body(const char *data, size_t len,
                                               char separator) {
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

template <class InputIterator>
InputIterator get_entity_in(InputIterator first, InputIterator last,
                            const std::string &id) {
    for (; first != last; first++) {
        if (*first != nullptr && (*first)->get_id() == id) {
            return first;
        }
    }
    return last;
}

void fail_if_bsid_not(BSID expected, BSID actual) {
    if (actual != expected) {
        throw std::runtime_error("unsupported BSID: " + std::to_string(actual));
    }
}

char *ti::write_len_header(size_t len) {
    char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
    int n = BYTES_LEN_HEADER - sizeof(len);
    while (n < BYTES_LEN_HEADER) {
        tsize[n] = (len >> (sizeof(len) - n - 1) * 8);
        n++;
    }
    return tsize;
}

size_t ti::read_len_header(const char *tsize) {
    int n = 0;
    size_t msize = 0;
    while (n < BYTES_LEN_HEADER) {
        msize <<= sizeof(char);
        msize |= tsize[n++];
    }
    return msize;
}

std::string ti::to_iso_time(const std::time_t &time) {
    char buf[sizeof "0000-00-00T00:00:00Z"]; // i hate magic numbers
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&time));
    return buf;
}

std::time_t ti::parse_iso_time(const std::string &str) {
    std::tm tm{};
    compat::time::strptime(str.c_str(), "%FT%TZ", &tm);
    return compat::time::timegm(&tm);
}

std::string write_entities(std::vector<Entity *> entities) {
    std::stringstream ss;
    std::transform(entities.begin(), entities.end(),
                   std::ostream_iterator<std::string>(ss, ","),
                   [&](Entity *e) { return e->get_id(); });
    auto str = ss.str();
    str = str.substr(0, str.length() - 1);
    return str;
}

bool Entity::operator==(const Entity &other) const {
    return other.get_id() == get_id();
}

Server::Server() = default;
std::string Server::get_id() const { return "zGuEzyj3EUyeSKAvHw3Zo"; }
size_t Server::serialize(char **dst) const {
    auto id = get_id();
    *dst = (char *)calloc(id.length() + 2, sizeof(char));
    (*dst)[0] = BSID::ENTY_SRV;
    std::memcpy(*dst + 1, id.c_str(), id.length());
    return id.length() + 1;
}

User::User(const std::string &id, const std::string &name,
           const std::string &bio, const time_t registration_time)
    : id(id), name(name), bio(bio), registration_time(registration_time) {}
std::string User::get_id() const { return id; }
std::string User::get_name() const { return name; }
std::string User::get_bio() const { return bio; }
time_t User::get_registration_time() const { return registration_time; }
size_t User::serialize(char **dst) const {
    auto reg_time = ti::to_iso_time(registration_time);
    auto len =
        id.length() + name.length() + bio.length() + reg_time.length() + 5;
    *dst = (char *)calloc(len, sizeof(char));
    (*dst)[0] = BSID::ENTY_USR;
    std::memcpy(*dst + 1, id.c_str(), id.length());
    std::memcpy(*dst + id.length() + 2, name.c_str(), name.length());
    std::memcpy(*dst + id.length() + name.length() + 3, bio.c_str(),
                bio.length());
    std::memcpy(*dst + len - reg_time.length() - 1, reg_time.c_str(),
                reg_time.length());
    return len;
}
User *User::deserialize(char *src, size_t len) {
    if (len <= 0) {
        return nullptr;
    }
    fail_if_bsid_not(BSID::ENTY_USR, (BSID)src[0]);
    auto args = read_message_body(src + 1, len - 1);
    if (args.size() != 4) {
        throw std::runtime_error("unexpected size (deserializing User)");
    }
    return new User(args[0], args[1], args[2], parse_iso_time(args[3]));
}

Group::Group(const std::string &id, const std::string &name,
             const std::vector<Entity *> &members)
    : id(id), name(name), members(members) {}
std::string Group::get_name() { return name; }
std::string Group::get_id() const { return id; }
std::vector<Entity *> &Group::get_members() { return members; }
size_t Group::serialize(char **dst) const {
    auto len = name.length() + id.length() + members.size() + 3;
    len += std::accumulate(
        members.begin(), members.end(), 0,
        [&](size_t l, const Entity *e) { return l + e->get_id().length(); });
    *dst = (char *)calloc(len, sizeof(char));
    (*dst)[0] = ENTY_GRP;
    std::memcpy(*dst + 1, id.c_str(), id.length());
    std::memcpy(*dst + id.length() + 2, name.c_str(), name.length());
    auto p = id.length() + name.length() + 3;
    for (auto e : members) {
        auto mid = e->get_id();
        std::memcpy(*dst + p, mid.c_str(), mid.length());
        p += mid.length() + 1;
    }
    return len;
}
Group *Group::deserialize(char *src, size_t len,
                          const std::vector<Entity *> &entities) {
    if (len <= 0) {
        return nullptr;
    }
    fail_if_bsid_not(BSID::ENTY_GRP, (BSID)src[0]);
    auto args = read_message_body(src + 1, len - 1);
    if (args.size() < 2) {
        throw std::runtime_error("unexpected size (deserializing Group)");
    }
    std::vector<Entity *> members;
    std::transform(
        args.begin() + 2, args.end(), std::back_inserter(members), [&](auto t) {
            return *get_entity_in(entities.begin(), entities.end(), t);
        });
    return new Group(args[0], args[1], members);
}

TextFrame::TextFrame(std::string id, std::string content)
    : id(std::move(id)), content(std::move(content)) {}
std::string TextFrame::get_id() const { return id; }
std::string TextFrame::to_string() const { return content; }
size_t TextFrame::serialize(char **dst) const {
    auto len = id.length() + content.length() + 3;
    *dst = (char *)calloc(len, sizeof(char));
    (*dst)[0] = BSID::FRM_TXT;
    std::memcpy(*dst + 1, id.c_str(), id.length());
    std::memcpy(*dst + id.length() + 2, content.c_str(), content.length());
    return len;
}
TextFrame *TextFrame::deserialize(char *src, size_t len) {
    if (len <= 0) {
        return nullptr;
    }
    fail_if_bsid_not(BSID::FRM_TXT, (BSID)src[0]);
    auto args = read_message_body(src + 1, len - 1);
    if (args.size() != 2) {
        throw std::runtime_error("unexpected size (deserializing TextFrame)");
    }
    return new TextFrame(args[0], args[1]);
}

Message::Message(const std::string &id, const std::vector<Frame *> &content,
                 std::time_t time, ti::Entity *sender, ti::Entity *receiver,
                 ti::Entity *forwared_from)
    : id(id), frames(content), time(time), sender(sender), receiver(receiver),
      forwarded_from(forwared_from) {}
const std::vector<Frame *> &Message::get_frames() const { return frames; }
std::string Message::get_id() const { return id; }
Entity *Message::get_sender() const { return sender; }
Entity *Message::get_receiver() const { return receiver; }
Entity *Message::get_forward_source() const { return forwarded_from; }
std::time_t Message::get_time() const { return time; }
bool Message::is_visible_by(const ti::Entity *entity) {
    if (receiver->get_id() == entity->get_id()) {
        return true;
    }
    if (auto g = dynamic_cast<Group *>(receiver)) {
        auto members = g->get_members();
        return std::find_if(members.begin(), members.end(), [&](Entity *e) {
                   return e->get_id() == entity->get_id();
               }) != members.end();
    }
    return false;
}
size_t Message::serialize(char **dst) const {
    auto time_str = ti::to_iso_time(time);
    auto forwardid = forwarded_from == nullptr ? "" : forwarded_from->get_id();

    size_t len = id.length() + BYTES_LEN_HEADER + sender->get_id().length() +
                 forwardid.length() + receiver->get_id().length() +
                 time_str.length() + 4;
    len += std::accumulate(frames.begin(), frames.end(), 0, [](auto a, auto e) {
        return a + e->get_id().length() + 1;
    });
    *dst = (char *)calloc(len, sizeof(char));

    size_t accu = 0;
    std::memcpy(*dst, id.c_str(), id.length());
    accu += id.length() + 1;

    auto forward_buf = write_len_header(frames.size());
    std::memcpy(*dst + accu, forward_buf, BYTES_LEN_HEADER);
    delete forward_buf;
    accu += BYTES_LEN_HEADER;

    std::string cid;
    for (auto frame : frames) {
        cid = frame->get_id();
        std::memcpy(*dst + accu, cid.c_str(), cid.length());
        accu += cid.length() + 1;
    }
    cid = sender->get_id();
    std::memcpy(*dst + accu, cid.c_str(), cid.length());
    accu += cid.length() + 1;
    cid = receiver->get_id();
    std::memcpy(*dst + accu, cid.c_str(), cid.length());
    accu += cid.length() + 1;
    std::memcpy(*dst + accu, forwardid.c_str(), forwardid.length());
    accu += forwardid.length() + 1;
    std::memcpy(*dst + accu, time_str.c_str(), time_str.length());

    return len;
}
Message *Message::deserialize(char *src, size_t len,
                              const std::vector<Frame *> &frames,
                              const std::vector<Entity *> &entities) {
    int ptr = 0;
    for (; ptr < len; ptr++) {
        if (src[ptr] == 0) {
            break;
        }
    }
    std::string id(src, src + ptr);
    ptr++;
    auto frame_count = read_len_header(src + ptr);
    std::vector<Frame *> content(frame_count);
    ptr += BYTES_LEN_HEADER;
    auto preptr = ptr;
    for (int i = 0; i < frame_count; ++i) {
        for (; ptr < len; ptr++) {
            if (src[ptr] == 0) {
                break;
            }
        }
        std::string cid(src + preptr, src + ptr);
        content[i] = *get_entity_in(frames.begin(), frames.end(), cid);
        if (content[i] == nullptr) {
            throw std::runtime_error("no such frame (deserializing Message)");
        }
        preptr = ++ptr;
    }

    auto args = read_message_body(src + ptr, len - ptr);
    if (args.size() != 4) {
        throw std::runtime_error("unexpected size (deserializing Message)");
    }
    return new Message(
        id, content, parse_iso_time(args[3]),
        *get_entity_in(entities.begin(), entities.end(), args[0]),
        *get_entity_in(entities.begin(), entities.end(), args[1]),
        args[2].length() <= 0
            ? nullptr
            : *get_entity_in(entities.begin(), entities.end(), args[2]));
}

SqlTransaction::SqlTransaction(const std::string &expr, sqlite3 *db)
    : closed(false) {
    int n =
        sqlite3_prepare_v2(db, expr.c_str(), expr.length(), &handle, nullptr);
    if (n != SQLITE_OK) {
        throw std::runtime_error("Invalid SQL expression");
    }
}
SqlTransaction::~SqlTransaction() { close(); }
void SqlTransaction::throw_on_fail(int code) {
    if (code != SQLITE_OK) {
        logD("[transition] bind failed with code %d", code);
        throw std::runtime_error("bind failed");
    }
}
void SqlTransaction::bind_text(int pos, const std::string &text) {
    throw_on_fail(sqlite3_bind_text(handle, pos + 1, text.c_str(),
                                    text.length(), nullptr));
}
void SqlTransaction::bind_int(int pos, const int n) {
    throw_on_fail(sqlite3_bind_int(handle, pos + 1, n));
}
void SqlTransaction::bind_int64(int pos, const long n) {
    throw_on_fail(sqlite3_bind_int64(handle, pos + 1, n));
}
void SqlTransaction::bind_double(int pos, double n) {
    throw_on_fail(sqlite3_bind_double(handle, pos + 1, n));
}
void SqlTransaction::bind_null(int pos) {
    throw_on_fail(sqlite3_bind_null(handle, pos + 1));
}
void SqlTransaction::bind_blob(int pos, void *blob, int nbytes) {
    throw_on_fail(sqlite3_bind_blob(handle, pos + 1, blob, nbytes, nullptr));
}
SqlTransaction::RowIterator SqlTransaction::begin() {
    return SqlTransaction::RowIterator(handle, false);
}
SqlTransaction::RowIterator SqlTransaction::end() {
    return SqlTransaction::RowIterator(handle, true);
}
void SqlTransaction::close() {
    if (!closed) {
        sqlite3_finalize(handle);
        closed = true;
    }
}

SqlTransaction::RowIterator::RowIterator(sqlite3_stmt *handle, bool end)
    : handle(handle), row(handle) {
    if (end) {
        rc = SQLITE_DONE;
        count = -1;
    } else {
        rc = sqlite3_step(handle);
        count = 0;
    }
}
SqlTransaction::RowIterator &SqlTransaction::RowIterator::operator++() {
    if (rc == SQLITE_ROW) {
        rc = sqlite3_step(handle);
        count++;
    }
    return *this;
}
SqlTransaction::RowIterator SqlTransaction::RowIterator::operator++(int) {
    return ++*this;
}
bool SqlTransaction::RowIterator::operator==(
    ti::orm::SqlTransaction::RowIterator other) const {
    return handle == other.handle &&
           (other.count < 0 && rc == SQLITE_DONE || count == other.count);
}
bool SqlTransaction::RowIterator::operator!=(
    ti::orm::SqlTransaction::RowIterator other) const {
    return !(*this == other);
}
SqlTransaction::RowIterator::reference
SqlTransaction::RowIterator::operator*() {
    return row;
}

Row::Row(sqlite3_stmt *handle) : handle(handle) {}
std::string Row::get_name(int col) const {
    return sqlite3_column_name(handle, col);
}
int Row::get_blob(int col, const void **rec) const {
    *rec = sqlite3_column_blob(handle, col);
    return sqlite3_column_bytes(handle, col);
}
double Row::get_double(int col) const {
    return sqlite3_column_double(handle, col);
}
int Row::get_int(int col) const { return sqlite3_column_int(handle, col); }
long Row::get_int64(int col) const { return sqlite3_column_int64(handle, col); }
int Row::get_type(int col) const { return sqlite3_column_type(handle, col); }
std::string Row::get_text(int col) const {
    const unsigned char *s = sqlite3_column_text(handle, col);
    if (s == nullptr) {
        throw std::overflow_error("column index overflow");
    }
    return reinterpret_cast<const char *>(s);
}

SqlDatabase::SqlDatabase(const std::string &dbfile) : is_cpy(false) {
    int n = sqlite3_open(dbfile.c_str(), &dbhandle);
    if (n != SQLITE_OK) {
        throw std::runtime_error("failed to open database");
    }
}
SqlDatabase::SqlDatabase(const ti::orm::SqlDatabase &h) : is_cpy(true) {
    dbhandle = h.dbhandle;
}
SqlDatabase::~SqlDatabase() {
    if (!is_cpy) {
        logD("[sql helper] closing db handle");
        sqlite3_close(dbhandle);
    }
}

void SqlDatabase::exec_sql(const std::string &expr) const {
    char *err;
    int n = sqlite3_exec(dbhandle, expr.c_str(), nullptr, nullptr, &err);
    if (n != SQLITE_OK) {
        auto m = std::string(err);
        throw std::runtime_error(m);
    }
}
int SqlDatabase::get_changes() const { return sqlite3_changes(dbhandle); }
SqlTransaction *SqlDatabase::prepare(const std::string &expr) const {
    return new SqlTransaction(expr, dbhandle);
}
void SqlDatabase::initialize() { sqlite3_initialize(); }
void SqlDatabase::shutdown() { sqlite3_shutdown(); }

TiOrm::TiOrm(const ti::orm::TiOrm &t) : SqlDatabase(t) {
    entities = t.entities;
    frames = t.frames;
    messages = t.messages;
}
TiOrm::TiOrm(const std::string &dbfile) : SqlDatabase(dbfile) {
    logD("[orm] executing initializing SQL");
    exec_sql(R"(CREATE TABLE IF NOT EXISTS "user"
(
    id                varchar(21) primary key not null,
    name              varchar                 not null,
    bio               varchar                 not null,
    registration_date datetime                not null
);
CREATE TABLE IF NOT EXISTS "group"
(
    id   varchar(21) primary key not null,
    name varchar                 not null,
    foreign key (id)
        references box (container_id)
        on delete cascade
);
CREATE TABLE IF NOT EXISTS "text_frame"
(
    id      varchar(21) primary key not null,
    content varchar                 not null
);
CREATE TABLE IF NOT EXISTS "message"
(
    id           varchar(21) primary key not null,
    time         datetime                not null,
    sender_id    varchar(21)             not null,
    receiver_id  varchar(21)             not null,
    forwarded_id varchar(21)             not null
);
CREATE TABLE IF NOT EXISTS "box"
(
    id           integer primary key,
    container_id varchar(21) not null,
    contained_id varchar(21) not null,
    unique (contained_id, container_id)
);
CREATE TABLE IF NOT EXISTS "contact"
(
    id         integer primary key,
    owner_id   varchar(21) not null,
    contact_id varchar(21) not null
);
)");
}
void TiOrm::pull() {
    entities.clear();
    entities.push_back(new Server());
    auto t = prepare(R"(SELECT * FROM "user")");
    for (auto row : *t) {
        entities.push_back(new User(row.get_text(0), row.get_text(1),
                                    row.get_text(2),
                                    parse_iso_time(row.get_text(3))));
    }
    delete t;

    t = prepare(R"(SELECT id, name FROM "group")");
    for (auto row : *t) {
        auto tr = prepare(
            R"(SELECT "contained_id" FROM "box" WHERE container_id = ?)");
        tr->bind_text(0, row.get_text(0));
        std::vector<Entity *> members;
        std::transform(tr->begin(), tr->end(), std::back_inserter(members),
                       [&](Row r) {
                           return *get_entity_in(entities.begin(),
                                                 entities.end(), r.get_text(0));
                       });
        entities.push_back(
            new Group(row.get_text(0), row.get_text(1), members));
    }
    delete t;

    frames.clear();
    t = prepare(R"(SELECT * FROM "text_frame")");
    for (auto row : *t) {
        frames.push_back(new TextFrame(row.get_text(0), row.get_text(1)));
    }
    delete t;

    messages.clear();
    t = prepare(R"(SELECT * FROM "message")");
    for (auto row : *t) {
        std::vector<Frame *> content;
        auto tr =
            prepare(R"(SELECT contained_id FROM "box" WHERE container_id = ?)");
        tr->bind_text(0, row.get_text(0));
        std::transform(tr->begin(), tr->end(), std::back_inserter(content),
                       [&](Row r) {
                           return *get_entity_in(frames.begin(), frames.end(),
                                                 r.get_text(0));
                       });
        messages.push_back(new Message(
            row.get_text(0), content, parse_iso_time(row.get_text(1)),
            *get_entity_in(entities.begin(), entities.end(), row.get_text(2)),
            *get_entity_in(entities.begin(), entities.end(), row.get_text(3)),
            *get_entity_in(entities.begin(), entities.end(), row.get_text(4))));
    }
    delete t;

    contacts.clear();
    t = prepare(R"(SELECT owner_id, contact_id FROM "contact")");
    for (auto e : *t) {
        contacts.emplace_back(get_user(e.get_text(0)),
                              *get_entity_in(entities.begin(), entities.end(), e.get_text(1)));
    }
    delete t;
}
TiOrm::~TiOrm() = default;
std::vector<User *> TiOrm::get_users() const {
    std::vector<User *> users;
    for (auto e : entities) {
        if (User *u = dynamic_cast<User *>(e)) {
            users.push_back(u);
        }
    }
    return users;
}
User *TiOrm::get_user(const std::string &id) const {
    User *u;
    for (auto e : entities) {
        if (e->get_id() == id && (u = dynamic_cast<User *>(e))) {
            return u;
        }
    }
    return nullptr;
}
std::vector<Entity *> TiOrm::get_contacts(const User *owner) const {
    std::vector<Entity *> ev;
    for (auto e : contacts) {
        if (*e.first == *owner) {
            ev.push_back(e.second);
        }
    }
    return ev;
}
void TiOrm::add_contact(User *owner, Entity *contact) {
    contacts.emplace_back(owner, contact);
    auto t = prepare(R"(INSERT INTO "contact"(owner_id, contact_id) VALUES (?, ?))");
    t->bind_text(0, owner->get_id());
    t->bind_text(1, contact->get_id());
    t->begin();
    delete t;
}
void TiOrm::add_entity(Entity *entity) {
    entities.push_back(entity);
    if (auto *u = dynamic_cast<User *>(entity)) {
        auto t = prepare(R"(INSERT INTO "user" VALUES (?, ?, ?, ?))");
        t->bind_text(0, u->get_id());
        t->bind_text(1, u->get_name());
        t->bind_text(2, u->get_bio());
        t->bind_text(3, to_iso_time(u->get_registration_time()));
        t->begin();
        delete t;
    } else if (auto *g = dynamic_cast<Group *>(entity)) {
        auto t = prepare(R"(INSERT INTO "group" VALUES (?, ?))");
        t->bind_text(0, g->get_id());
        t->bind_text(1, g->get_name());
        t->begin();
        for (auto m : g->get_members()) {
            t = prepare(
                R"(INSERT INTO "box"(container_id, contained_id) VALUES (?, ?))");
            t->bind_text(0, g->get_id());
            t->bind_text(1, m->get_id());
            t->begin();
        }
        delete t;
    } else {
        throw std::runtime_error("entity type not implemented");
    }
}
void TiOrm::delete_entity(ti::Entity *entity) {
    auto r = std::remove(entities.begin(), entities.end(), entity);
    if (*r == nullptr) {
        throw std::runtime_error("entity not found");
    }
    SqlTransaction *t;
    if (auto *u = dynamic_cast<User *>(entity)) {
        t = prepare(R"(DELETE FROM "user" WHERE id = ?)");
    } else if (auto *g = dynamic_cast<Group *>(entity)) {
        t = prepare(R"(DELETE FROM "group" WHERE id = ?)");
    } else {
        throw std::runtime_error("unsupported enetity type");
    }
    t->bind_text(0, entity->get_id());
    t->begin();
    delete t;
}
std::vector<Entity *> TiOrm::get_entities() const { return entities; }
Entity *TiOrm::get_entity(const std::string &id) const {
    return *get_entity_in(entities.begin(), entities.end(), id);
}
void TiOrm::add_frames(const std::vector<Frame *> &frm, Message *parent) {
    for (auto f : frm) {
        frames.push_back(f);
        if (parent != nullptr) {
            auto t = prepare(
                R"(INSERT INTO "box"(container_id, contained_id) VALUES (?, ?))");
            t->bind_text(0, parent->get_id());
            t->bind_text(1, f->get_id());
            t->begin();
            delete t;
        }
        if (auto *tf = dynamic_cast<TextFrame *>(f)) {
            auto t = prepare(R"(INSERT INTO "text_frame" VALUES (?, ?))");
            t->bind_text(0, tf->get_id());
            t->bind_text(1, tf->to_string());
            t->begin();
            delete t;
        } else {
            throw std::runtime_error("unimplemented frame type");
        }
    }
}
std::vector<Message *> TiOrm::get_messages() const { return messages; }
Message *TiOrm::get_message(const std::string &id) {
    return *get_entity_in(messages.begin(), messages.end(), id);
}
void TiOrm::add_message(ti::Message *msg) {
    messages.push_back(msg);
    add_frames(msg->get_frames(), msg);
    auto t = prepare(R"(INSERT INTO "message" VALUES (?, ?, ?, ?, ?))");
    t->bind_text(0, msg->get_id());
    t->bind_text(1, to_iso_time(msg->get_time()));
    t->bind_text(2, msg->get_sender()->get_id());
    t->bind_text(3, msg->get_receiver()->get_id());
    t->bind_text(4, msg->get_receiver()->get_id());
    t->begin();
    delete t;
}
