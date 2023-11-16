#include "protocol.hpp"
#include "log.hpp"

#define BUFFER_SIZE 3

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

bool Entity::operator==(ti::Entity &other) const {
    return other.get_id() == get_id();
}

Server::Server() = default;
std::string Server::get_id() const { return "zGuEzyj3EUyeSKAvHw3Zo"; }

User::User(std::string id, std::string name)
    : id(std::move(id)), name(std::move(name)) {}
std::string User::get_id() const { return id; }
std::string User::get_name() { return name; }

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
std::vector<Frame *> &Message::get_frames() { return frames; }
std::string Message::get_id() { return id; }
Entity &Message::get_sender() { return *sender; }
Entity &Message::get_receiver() { return *receiver; }

SqlTransaction::SqlTransaction(const std::string &expr, sqlite3 *db) {
    int n =
        sqlite3_prepare_v2(db, expr.c_str(), expr.length(), &handle, nullptr);
    if (n != SQLITE_OK) {
        throw std::runtime_error("Invalid SQL expression");
    }
}
SqlTransaction::~SqlTransaction() { sqlite3_finalize(handle); }
void SqlTransaction::throw_on_fail(int code) {
    if (code != SQLITE_OK) {
        throw std::runtime_error("Bind failed");
    }
}
void SqlTransaction::bind_text(int pos, const std::string &text) {
    throw_on_fail(
        sqlite3_bind_text(handle, pos, text.c_str(), text.length(), nullptr));
}
void SqlTransaction::bind_int(int pos, const int n) {
    throw_on_fail(sqlite3_bind_int(handle, pos, n));
}
void SqlTransaction::bind_int64(int pos, const long n) {
    throw_on_fail(sqlite3_bind_int64(handle, pos, n));
}
void SqlTransaction::bind_double(int pos, double n) {
    throw_on_fail(sqlite3_bind_double(handle, pos, n));
}
void SqlTransaction::bind_null(int pos) {
    throw_on_fail(sqlite3_bind_null(handle, pos));
}
void SqlTransaction::bind_blob(int pos, void *blob, int nbytes) {
    throw_on_fail(sqlite3_bind_blob(handle, pos, blob, nbytes, nullptr));
}
SqlTransaction::RowIterator SqlTransaction::begin() {
    return SqlTransaction::RowIterator(handle, false);
}
SqlTransaction::RowIterator SqlTransaction::end() {
    return SqlTransaction::RowIterator(handle, true);
}

SqlTransaction::RowIterator::RowIterator(sqlite3_stmt *handle, bool end)
    : handle(handle), rc(end ? SQLITE_DONE : SQLITE_ROW), row(handle) {
    if (end) {
        count = sqlite3_column_count(handle);
    } else {
        count = 0;
    }
}
SqlTransaction::RowIterator &SqlTransaction::RowIterator::operator++() {
    if (rc == SQLITE_ROW) {
        rc = sqlite3_step(handle);
    }
    return *this;
}
SqlTransaction::RowIterator SqlTransaction::RowIterator::operator++(int) {
    return ++*this;
}
bool SqlTransaction::RowIterator::operator==(
    ti::orm::SqlTransaction::RowIterator other) const {
    return handle == other.handle && count == other.count;
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
std::string Row::get_text(int col) {
    const unsigned char *s = sqlite3_column_text(handle, col);
    return reinterpret_cast<const char *>(s);
}

typedef struct {
    Table *table;
    int cap;
} TableProcessor;

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

int callback(void *t, int argc, char **argv, char **azColName) {
    auto *tp = (TableProcessor *)t;
    auto tt = tp->table;
    if (tt->width <= 0) {
        tt->width = argc;
    }
    if (tt->columns == nullptr) {
        tt->columns = (std::string *)malloc(sizeof(std::string *) * argc);
        for (int i = 0; i < argc; ++i) {
            tt->columns[i] = std::string(azColName[i]);
        }
    }

    if (tp->cap <= tt->height) {
        if (tt->rows == nullptr) {
            tt->rows = (std::string **)calloc(tp->cap, sizeof(std::string *));
        } else {
            tt->rows = (std::string **)realloc(tt->rows,
                                               sizeof(std::string *) * tp->cap);
            if (tt->rows == nullptr) {
                throw std::runtime_error("realloc() failed");
            }
        }
        tp->cap += BUFFER_SIZE;
    }
    tt->rows[tt->height] = (std::string *)malloc(sizeof(std::string *));
    for (int i = 0; i < argc; ++i) {
        tt->rows[tt->height][i] = std::string(argv[i]);
    }
    tt->height++;
    return 0;
}
Table *SqlDatabase::exec_sql(const std::string &expr) {
    auto *tt = (Table *)malloc(sizeof(Table));
    tt->height = 0;
    tt->width = 0;
    tt->rows = nullptr;
    tt->columns = nullptr;
    auto *tp = (TableProcessor *)malloc(sizeof(TableProcessor));
    tp->table = tt;
    tp->cap = 0;
    char *err = nullptr;
    int n = sqlite3_exec(dbhandle, expr.c_str(), callback, tp, &err);
    if (n == SQLITE_OK) {
        return tt;
    } else {
        auto m = std::string(err);
        sqlite3_free(err);
        throw std::runtime_error(m);
    }
}
void SqlDatabase::exec_sql_no_result(const std::string &expr) {
    char *err;
    int n = sqlite3_exec(dbhandle, expr.c_str(), nullptr, nullptr, &err);
    if (n != SQLITE_OK) {
        auto m = std::string(err);
        throw std::runtime_error(m);
    }
}
SqlTransaction *SqlDatabase::prepare(const std::string &expr) {
    return new SqlTransaction(expr, dbhandle);
}
void SqlDatabase::initialize() { sqlite3_initialize(); }
void SqlDatabase::shutdown() { sqlite3_shutdown(); }

Contact::Contact(User *owner, Entity *contact)
    : owner(owner), contact(contact) {}
Entity *Contact::get_contact() const { return contact; }
Entity *Contact::get_owner() const { return owner; }

static std::string init_sql() {
    return "CREATE TABLE IF NOT EXISTS \"user\"\n"
           "(\n    id   varchar(21) primary key not null,\n"
           "    name varchar                 not null\n"
           ");\n"
           "CREATE TABLE IF NOT EXISTS \"group\"\n"
           "(\n"
           "    id         varchar(21) primary key not null,\n"
           "    name       varchar                 not null,\n"
           "    members_id varchar                 not null\n"
           ");\n"
           "CREATE TABLE IF NOT EXISTS \"text_frame\"\n"
           "(\n"
           "    id      varchar(21) primary key not null,\n"
           "    content varchar                 not null\n"
           ");\n"
           "CREATE TABLE IF NOT EXISTS \"message\"\n"
           "(\n"
           "    id           varchar(21) primary key not null,\n"
           "    frames_id    varchar                 not null,\n"
           "    time         datetime                not null,\n"
           "    sender_id    varchar(21)             not null,\n"
           "    receiver_id  varchar(21)             not null,\n    "
           "    forwarded_id varchar(21)             not null\n"
           ");\n"
           "";
}
TiOrm::TiOrm(const ti::orm::TiOrm &t) : SqlDatabase(t) {
    logD("[orm (copied)] executing initializing SQL");
    exec_sql_no_result(init_sql());
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
template <typename T, typename F>
std::vector<T> maps(const std::string &str, const char separator,
                    F mapping_fn) {
    std::vector<T> ss;
    int j, k;
    for (j = 0, k = 0; j < str.length(); ++j) {
        if (str[j] == separator) {
            auto curr = str.substr(k, j);
            k = j + 1;
            ss.push_back(mapping_fn(curr));
        }
    }
    return ss;
}
TiOrm::TiOrm(const std::string &dbfile) : SqlDatabase(dbfile) {
    logD("[orm] executing initializing SQL");
    exec_sql_no_result(init_sql());
    auto t = exec_sql("SELECT * FROM \"user\"");
    for (int i = 0; i < t->height; ++i) {
        auto row = t->rows[i];
        entities.push_back(new User(row[0], row[1]));
    }
    delete t;
    t = exec_sql("SELECT * FROM \"group\"");
    for (int i = 0; i < t->height; ++i) {
        auto row = t->rows[i];
        auto ms = row[2];
        auto members = maps<Entity *>(ms, ',', [&](std::string &id) {
            return get_entity_in(entities, id);
        });
        entities.push_back(new Group(row[0], row[1], members));
    }
    delete t;
    t = exec_sql("SELECT * FROM \"text_frame\"");
    for (int i = 0; i < t->height; ++i) {
        auto row = t->rows[i];
        frames.push_back(new TextFrame(row[0], row[1]));
    }
    delete t;
    auto transaction = prepare("SELECT * FROM \"message\"");
    for (auto row : *transaction) {
        auto fs = row.get_text(1);
        auto content =
            maps<Frame *>(row.get_text(1), ',', [&](std::string &id) {
                return get_entity_in(frames, id);
            });
        messages.push_back(
            new Message(row.get_text(0), content, row.get_int64(2),
                        get_entity_in(entities, row.get_text(3)),
                        get_entity_in(entities, row.get_text(4)),
                        get_entity_in(entities, row.get_text(5))));
    }
    delete transaction;
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
const Entity *TiOrm::get_entity(const std::string &id) const {
    for (auto e : entities) {
        if (e->get_id() == id) {
            return e;
        }
    }
    return nullptr;
}
const std::vector<Message *> &TiOrm::get_messages() const { return messages; }

ServerOrm::ServerOrm(const std::string &dbfile) : TiOrm(dbfile) {}
const std::vector<Contact *> &ServerOrm::get_contacts() { return contacts; }
std::vector<Entity *> ServerOrm::get_contacts(User *owner) {
    std::vector<Entity *> ev;
    for (auto e : contacts) {
        if (e->get_owner() == owner) {
            ev.push_back(e->get_contact());
        }
    }
    return ev;
}
