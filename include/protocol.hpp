#include "nanoid/nanoid.h"
#include <iostream>
#include <sqlite3.h>
#include <vector>

#define BYTES_LEN_HEADER 8

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define closesocketfd(A)                                                       \
    closesocket(A);                                                            \
    WSACleanup()
#define SocketFd SOCKET
#else
#define closesocketfd(A) close(A)
#define SocketFd int
#endif

namespace ti {
const std::string version = "0.1";

char *write_len_header(size_t len);
size_t read_len_header(char *tsize);

class Entity {
  public:
    virtual std::string get_id() = 0;
};

class Server : Entity {
  public:
    std::string get_id() override;
    Server();
};

class User : public Entity {
    std::string name, id;

  public:
    User(std::string id, std::string name);

    std::string get_id() override;
    std::string get_name();
};

class Group : public Entity {
    std::string name, id;
    std::vector<Entity *> members;

  public:
    Group(std::string id, std::string name, std::vector<Entity *> members);

    std::string get_id() override;
    std::string get_name();
    std::vector<Entity *> &get_members();
};

class Frame {
  public:
    virtual std::string to_string() = 0;
    virtual std::string get_id() = 0;
};

class TextFrame : public Frame {
    std::string content;
    std::string id;

  public:
    TextFrame(std::string id, std::string content);
    std::string get_id() override;
    std::string to_string() override;
};

class Message {
    std::vector<Frame *> frames;
    Entity *sender, *receiver, *forwarded_from;
    std::string id;
    std::time_t time;

  public:
    Message(std::string id, std::vector<Frame *> content, std::time_t time,
            Entity *sender, Entity *receiver, Entity *forwared_from);
    std::vector<Frame *> &get_frames();
    std::string get_id();
    Entity &get_sender();
    Entity &get_receiver();
};

enum RequestType {
    LOGIN = 0x00,
};

namespace orm {
typedef struct {
    int height, width;
    std::string *columns;
    std::string **rows;
} Table;

class Row {
    sqlite3_stmt *handle;

  public:
    explicit Row(sqlite3_stmt *handle);
    std::string get_text(int col);
    int get_int(int col);
    long get_int64(int col);
    int get_blob(int col, const void **rec);
    double get_double(int col);
    std::string get_name(int col);
    int get_type(int col);
};

class SqlTransaction {
    sqlite3_stmt *handle;
    static void throw_on_fail(int code);

  public:
    SqlTransaction(const std::string &expr, sqlite3 *db);
    ~SqlTransaction();
    void bind_text(int pos, const std::string &text);
    void bind_int(int pos, int n);
    void bind_int64(int pos, long n);
    void bind_blob(int pos, void *blob, int nbytes);
    void bind_double(int pos, double n);
    void bind_null(int pos);

    class RowIterator : public std::iterator<std::input_iterator_tag, Row, int,
                                             const Row *, const Row &> {
        sqlite3_stmt *handle;
        int rc, count;
        const Row row;

      public:
        explicit RowIterator(sqlite3_stmt *handle, bool end = false);
        RowIterator &operator++();
        RowIterator operator++(int);
        bool operator==(RowIterator other) const;
        bool operator!=(RowIterator other) const;
        reference operator*();
    };
    RowIterator begin();
    RowIterator end();
};

/**
 * C++ wrapper for SQLite 3 C API
 */
class SqlDatabase {
    sqlite3 *dbhandle;
    bool is_cpy;

  protected:
    SqlTransaction *prepare(const std::string &expr);
    Table *exec_sql(const std::string &expr);
    void exec_sql_no_result(const std::string &expr);

  public:
    explicit SqlDatabase(const std::string &dbfile);
    SqlDatabase(const SqlDatabase &h);
    ~SqlDatabase();
    static void initialize();
    static void shutdown();
};

class Contact {
    User *owner;
    Entity *contact;

  public:
    Contact(User *owner, Entity *contact);
    Entity *get_owner();
    Entity *get_contact();
};

class TiOrm : SqlDatabase {
    std::vector<Entity *> entities;
    std::vector<Frame *> frames;
    std::vector<Message *> messages;

  public:
    explicit TiOrm(const std::string &dbfile);
    TiOrm(const TiOrm &t);
    ~TiOrm();
    const std::vector<User *> get_users();
    const Entity *get_entity(const std::string &id);
    const std::vector<Message *> &get_messages();
};

class ServerOrm : public TiOrm {
    std::vector<Contact *> contacts;

  public:
    explicit ServerOrm(const std::string &dbfile);
    ServerOrm(const ServerOrm &t);
    const std::vector<Contact *> &get_contacts();
    const std::vector<Entity *> get_contacts(User *owner);
};
} // namespace orm
} // namespace ti
