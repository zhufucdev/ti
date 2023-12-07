#include "socketcompat.h"
#include <ctime>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace ti {
const std::string version = "0.1";

class BinarySerializable {
  public:
    virtual size_t serialize(char **dst) const = 0;
};

enum BSID { ENTY_SRV = 0x00, ENTY_USR, ENTY_GRP, FRM_TXT = 0x40 };

class Entity : public BinarySerializable {
  public:
    virtual ~Entity() = default;
    virtual std::string get_id() const = 0;
    bool operator==(const Entity &other) const;
    static Entity *deserialize(char *src, size_t len,
                               const std::function<Entity *(const std::string &)> &getter);
};

class Server final : public Entity {
  public:
    std::string get_id() const override;
    ~Server() final;
    Server();
    size_t serialize(char **dst) const override;
    static Server INSTANCE;
};

class User : public Entity {
    std::string name, id, bio;
    time_t registration_time;

  public:
    User(const std::string &id, const std::string &name, const std::string &bio,
         time_t registration_time);

    std::string get_id() const override;
    std::string get_name() const;
    std::string get_bio() const;
    time_t get_registration_time() const;
    size_t serialize(char **dst) const override;
    static User *deserialize(char *src, size_t len);
};

class Group : public Entity {
    std::string name, id;
    std::vector<Entity *> members;

  public:
    Group(const std::string &id, const std::string &name,
          const std::vector<Entity *> &members);

    std::string get_id() const override;
    std::string get_name();
    std::vector<Entity *> &get_members();
    size_t serialize(char **dst) const override;
    static Group *deserialize(char *src, size_t len,
                              const std::function<Entity *(const std::string &)> &getter);
};

class Frame : public BinarySerializable {
  public:
    virtual ~Frame() = default;
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
    size_t serialize(char **dst) const override;
    static TextFrame *deserialize(char *src, size_t len);
};

class Message : public BinarySerializable {
    std::vector<Frame *> frames;
    Entity *sender, *receiver, *forwarded_from;
    std::string id;
    std::time_t time;

  public:
    Message(const std::string &id, const std::vector<Frame *> &content,
            std::time_t time, Entity *sender, Entity *receiver,
            Entity *forwared_from);
    const std::vector<Frame *> &get_frames() const;
    std::string get_id() const;
    Entity *get_sender() const;
    Entity *get_receiver() const;
    Entity *get_forward_source() const;
    std::time_t get_time() const;
    bool is_visible_by(const Entity *entity);
    std::vector<User *> get_all_receivers();
    size_t serialize(char **dst) const override;
    static Message *deserialize(char *src, size_t len,
                                const std::vector<Frame *> &frames,
                                const std::vector<Entity *> &entities);
};

enum RequestCode {
    LOGIN = 0,
    LOGOUT,
    REGISTER,
    SYNC,
    DELETE_USER,
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
    int get_blob(int col, void **rec) const;
    double get_double(int col) const;
    std::string get_name(int col) const;
    int get_type(int col) const;
};

class SqlTransaction {
    sqlite3_stmt *handle;
    bool closed;
    std::vector<char *> pending_str;
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

  public:
    explicit SqlDatabase(const std::string &dbfile);
    SqlDatabase(const SqlDatabase &h);
    ~SqlDatabase();
    SqlTransaction *prepare(const std::string &expr) const;
    void exec_sql(const std::string &expr) const;
    int get_changes() const;

    static void initialize();
    static void shutdown();
};

struct ByteArray {
    size_t len;
    char *hash;
};

class Sync {
    SqlDatabase *db;
    User *owner;
    ByteArray *ch, *mh;
    std::vector<SqlTransaction *> pending;
    bool *destroyed;

  public:
    Sync(SqlDatabase *db, User *owner);
    ~Sync();
    const ByteArray *get_contacts_hash();
    const ByteArray *get_messages_hash();
};

class TiOrm : public SqlDatabase {
    std::vector<Entity *> entities;
    std::vector<Frame *> frames;
    std::vector<Message *> messages;
    std::vector<std::pair<User *, Entity *>> contacts;

    void reset();
    void update_sync(const User *owner, const std::string& addition, std::string field);

  public:
    explicit TiOrm(const std::string &dbfile);
    TiOrm(const TiOrm &t);
    ~TiOrm();
    virtual void pull();
    std::vector<User *> get_users() const;
    User *get_user(const std::string &id) const;
    std::vector<Entity *> get_contacts(User *owner) const;
    void add_contact(User *owner, Entity *contact);
    bool delete_contact(User *owner, Entity *contact);
    /**
     * Insert a new entity, or replace the existing one,
     * making whose pointer invalid
     * @param entity
     */
    void add_entity(Entity *entity);
    void delete_entity(Entity *entity);
    std::vector<Entity *> get_entities() const;
    Entity *get_entity(const std::string &id) const;
    void add_frames(const std::vector<Frame *> &frm, Message *parent = nullptr);
    std::vector<Message *> get_messages() const;
    Message *get_message(const std::string &id);
    void add_message(Message *msg);
    bool delete_message(Message *msg);
    Sync get_sync(ti::User *owner) const;
};
} // namespace orm
} // namespace ti
