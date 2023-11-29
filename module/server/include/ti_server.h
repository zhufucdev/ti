#include "server.h"

namespace ti {
namespace server {
class ServerOrm : public orm::TiOrm {
    std::vector<std::pair<User *, Entity *>> contacts;
    std::vector<std::pair<User *, std::string>> tokens;

  public:
    explicit ServerOrm(const std::string &dbfile);
    void pull() override;
    std::vector<Entity *> get_contacts(const User *owner) const;
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
    /**
     * Response code: OK, NOT_FOUND
     * @param user_id
     * @param password
     */
    void user_login(const std::string &user_id, const std::string &password);
    /**
     * Response code: OK, NOT_FOUND
     * @param old_token
     */
    void reconnect(const std::string &old_token);
    /**
     * Align the local and remote database
     * Response code: TODO
     * @param curr_token
     * @param selector
     */
    void sync(const std::string &curr_token, const std::string &selector);
    /**
     * Unregister current account
     * Response code: TOKEN_EXPIRED, OK, NOT_FOUND
     * @param curr_token
     */
    void user_delete(const std::string &curr_token);
    /**
     * End an another remote session
     * Response code: TOKEN_EXPIRED, OK, NOT_FOUND
     * @param curr_token
     * @param token_id
     */
    void determine(const std::string &curr_token, int token_id);
    /**
     * Response code: TOKEN_EXPIRED, OK, NOT_FOUND
     * @param curr_token
     */
    void logout(const std::string &curr_token);
    /**
     * Response code: OK, BAD_REQUEST
     * @param user_name
     * @param passcode
     */
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
