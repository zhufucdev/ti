#include "ti.h"
#include "log.h"
#include <sstream>

using namespace ti;
using namespace orm;

char *ti::write_len_header(size_t len) {
    char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
    int n = BYTES_LEN_HEADER - sizeof(len);
    while (n < BYTES_LEN_HEADER) {
        tsize[n] = (len >> (sizeof(len) - n - 1) * 8);
        n++;
    }
    return tsize;
}

size_t ti::read_len_header(char *tsize) {
    int n = 0;
    size_t msize = 0;
    while (n < BYTES_LEN_HEADER) {
        msize <<= sizeof(char);
        msize |= tsize[n++];
    }
    return msize;
}

std::string ti::to_iso_time(const std::time_t &time) {
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&time));
    return buf;
}

std::time_t ti::parse_iso_time(const std::string &str) {
    std::tm tm{};
    strptime(str.c_str(), "%FT%TZ", &tm);
    return timegm(&tm);
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

User::User(const std::string &id, const std::string &name,
           const std::string &bio, const time_t registration_time)
    : id(id), name(name), bio(bio), registration_time(registration_time) {}
std::string User::get_id() const { return id; }
std::string User::get_name() const { return name; }
std::string User::get_bio() const { return bio; }
time_t User::get_registration_time() const { return registration_time; }

Group::Group(std::string id, std::string name, std::vector<Entity *> members)
    : id(std::move(id)), name(std::move(name)), members(std::move(members)) {}
std::string Group::get_name() { return name; }
std::string Group::get_id() const { return id; }
std::vector<Entity *> &Group::get_members() { return members; }

TextFrame::TextFrame(std::string id, std::string content)
    : id(std::move(id)), content(std::move(content)) {}
std::string TextFrame::get_id() const { return id; }
std::string TextFrame::to_string() const { return content; }

Message::Message(std::string id, std::vector<Frame *> content, std::time_t time,
                 ti::Entity *sender, ti::Entity *receiver,
                 ti::Entity *forwared_from)
    : id(std::move(id)), frames(std::move(content)), time(time), sender(sender),
      receiver(receiver), forwarded_from(forwared_from) {}
const std::vector<Frame *> &Message::get_frames() const { return frames; }
std::string Message::get_id() const { return id; }
Entity *Message::get_sender() const { return sender; }
Entity *Message::get_receiver() const { return receiver; }
Entity *Message::get_forward_source() const { return forwarded_from; }
std::time_t Message::get_time() const { return time; }

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
template <typename T>
T *get_entity_in(const std::vector<T *> &vec, const std::string &id) {
    for (auto e : vec) {
        if (e->get_id() == id) {
            return e;
        }
    }
    throw std::runtime_error("specific entity not found");
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
);)");
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
        std::transform(
            tr->begin(), tr->end(), std::back_inserter(members),
            [&](Row r) { return get_entity_in(entities, r.get_text(0)); });
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
        std::transform(
            tr->begin(), tr->end(), std::back_inserter(content),
            [&](Row r) { return get_entity_in(frames, r.get_text(0)); });
        messages.push_back(new Message(
            row.get_text(0), content, parse_iso_time(row.get_text(1)),
            get_entity_in(entities, row.get_text(2)),
            get_entity_in(entities, row.get_text(3)),
            get_entity_in(entities, row.get_text(4))));
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
    return get_entity_in(entities, id);
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
    return get_entity_in(messages, id);
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
