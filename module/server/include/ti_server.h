#include "server.h"

namespace ti {
namespace server {
class ServerOrm : public orm::TiOrm {
    std::vector<std::pair<User *, Entity *>> contacts;
    std::vector<std::pair<User *, std::string>> tokens;

  public:
    explicit ServerOrm(const std::string &dbfile);
    void pull() override;
    std::vector<Entity *> get_contacts(const User &owner) const;
    bool check_password(const std::string &user_id, const std::string &passcode) const;
    User *check_token(const std::string &token) const;
    void add_token(ti::User *owner, const std::string &token);
    bool invalidate_token(int token_id, User *owner = nullptr);
    bool invalidate_token(const std::string &token, User *owner = nullptr);
    void add_user(User *user, const std::string &passcode);
};
class TiServer : public Server {
    ServerOrm db;

  public:
    TiServer(std::string addr, short port, const std::string &dbfile);
    ~TiServer();
    Client *on_connect(sockaddr_in addr) override;
};
class TiClient : public Client {
    ServerOrm &db;
    std::string id;
    User *user;
    std::string token;
    void user_login(const std::string &user_id, const std::string &password);
    void reconnect(const std::string &old_token);
    void user_delete(const std::string &curr_token);
    void determine(const std::string &curr_token, int token_id);
    void logout(const std::string &old_token);
    void user_register(const std::string &user_name, const std::string &passcode);

  public:
    explicit TiClient(ServerOrm &db);
    ~TiClient() override;
    void on_connect(sockaddr_in addr) override;
    void on_disconnect() override;
    void on_message(RequestCode req, char *data, size_t len) override;
};
} // namespace server
} // namespace ti
