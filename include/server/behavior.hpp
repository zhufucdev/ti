#include "protocol.hpp"

namespace ti {
namespace server {
class ServerOrm : public orm::TiOrm {
    std::vector<std::pair<User *, Entity *>> contacts;
    std::vector<std::pair<User *, std::string>> tokens;

  public:
    explicit ServerOrm(const std::string &dbfile);
    void pull() override;
    std::vector<Entity *> get_contacts(const User &owner) const;
    int get_password(const std::string &user_id, const void **buf) const;
    User *check_token(const std::string &token) const;
    void add_token(ti::User *owner, const std::string &token);
    bool invalidate_token(int token_id, User *owner = nullptr);
    bool invalidate_token(const std::string &token);
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
    void login(const std::string &user_id, const std::string &passcode);
    void reconnect(const std::string &old_token);
    void determine(const std::string &curr_token, int token_id);
    void logout();

  public:
    explicit TiClient(ServerOrm &db);
    ~TiClient() override;
    void on_connect(sockaddr_in addr) override;
    void on_disconnect() override;
    void on_message(RequestCode req, char *data, size_t len) override;
};
} // namespace server
} // namespace ti
