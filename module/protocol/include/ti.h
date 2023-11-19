#include <sqlite3.h>
#include <string>
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
std::string to_iso_time(const std::time_t &time);
std::time_t parse_iso_time(const std::string& str);

class Entity {
  public:
    virtual std::string get_id() const = 0;
    bool operator==(const Entity &other) const;
};

class Server : public Entity {
  public:
    std::string get_id() const override;
    Server();
};

class User : public Entity {
    std::string name, id, bio;

  public:
    User(const std::string &id, const std::string &name,
         const std::string &bio);

    std::string get_id() const override;
    std::string get_name() const;
    std::string get_bio() const;
};

class Group : public Entity {
    std::string name, id;
    std::vector<Entity *> members;

  public:
    Group(std::string id, std::string name, std::vector<Entity *> members);

    std::string get_id() const override;
    std::string get_name();
    std::vector<Entity *> &get_members();
};

class Frame {
  public:
    virtual std::string to_string() const = 0;
    virtual std::string get_id() const = 0;
};

class TextFrame : public Frame {
    std::string content;
    std::string id;

  public:
    TextFrame(std::string id, std::string content);
    std::string get_id() const override;
    std::string to_string() const override;
};

class Message {
    std::vector<Frame *> frames;
    Entity *sender, *receiver, *forwarded_from;
    std::string id;
    std::time_t time;

  public:
    Message(std::string id, std::vector<Frame *> content, std::time_t time,
            Entity *sender, Entity *receiver, Entity *forwared_from);
    const std::vector<Frame *> &get_frames() const;
    std::string get_id() const;
    Entity *get_sender() const;
    Entity *get_receiver() const;
    Entity *get_forward_source() const;
    std::time_t get_time() const;
};

enum RequestCode {
    LOGIN = 0,
    LOGOUT,
    REGISTER,
    RECONNECT,
    DETERMINE,
};
enum ResponseCode { OK = 0, NOT_FOUND, BAD_REQUEST, TOKEN_EXPIRED, MESSAGE };

namespace orm {
class Row {
    sqlite3_stmt *handle;

  public:
    explicit Row(sqlite3_stmt *handle);
    std::string get_text(int col) const;
    int get_int(int col) const;
    long get_int64(int col) const;
    int get_blob(int col, const void **rec) const;
    double get_double(int col) const;
    std::string get_name(int col) const;
    int get_type(int col) const;
};

class SqlTransaction {
    sqlite3_stmt *handle;
    bool closed;
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
    void close();

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
 * Wrapper for SQLite 3 C API
 */
class SqlDatabase {
    sqlite3 *dbhandle;
    bool is_cpy;

  protected:
    SqlTransaction *prepare(const std::string &expr) const;
    void exec_sql(const std::string &expr) const;
    int get_changes() const;

  public:
    explicit SqlDatabase(const std::string &dbfile);
    SqlDatabase(const SqlDatabase &h);
    ~SqlDatabase();
    static void initialize();
    static void shutdown();
};

class TiOrm : public SqlDatabase {
    std::vector<Entity *> entities;
    std::vector<Frame *> frames;
    std::vector<Message *> messages;

  public:
    explicit TiOrm(const std::string &dbfile);
    TiOrm(const TiOrm &t);
    ~TiOrm();
    virtual void pull();
    std::vector<User *> get_users() const;
    User *get_user(const std::string &id) const;
    void add_entity(Entity *entity);
    std::vector<Entity *> get_entities() const;
    Entity *get_entity(const std::string &id) const;
    void add_frames(const std::vector<Frame *>& frm, Message *parent = nullptr);
    std::vector<Message *> get_messages() const;
    Message *get_message(const std::string &id);
    void add_message(Message *msg);
};
} // namespace orm
} // namespace ti
