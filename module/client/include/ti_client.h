#include "client.h"

namespace ti {
namespace client {

enum TiClientState { OFFLINE, LOGGED_OUT, READY };

class TiClient : public Client, public orm::TiOrm {
    TiClientState state;
    std::string userid, token;

    void panic_if_not(ti::client::TiClientState target);

  public:
    TiClient(std::string addr, short port, const std::string &dbfile);
    ~TiClient();
    TiClientState get_state();
    bool try_reconnect() noexcept;
    bool reconnect(const std::string &new_token = {});
    bool sync();
    bool user_login(const std::string &user_id, const std::string &password);
    std::string user_reg(const std::string &name, const std::string &password);
    bool user_logout();
    bool user_delete();
    User *get_current_user() const;
    /**
     * Get an entity. If the entity was not found in local cache,
     * an attempt to download from the server would be launched.
     * @param id entity id
     * @return entity pointer, whose lifespan is most possibly
     * the same as this client instance
     */
    Entity *get_entity_or_download(const std::string &id);
    /**
     * Get a message. If the message was not cached, an attempt
     * to download from the server would be launched.
     * @param id message id
     * @return pointer to this message, whose lifespan is most
     * possibly the same as this client instance
     */
    Message *get_message_or_download(const std::string &id);
    std::vector<Entity *> get_contacts() const;
    void send(Message *message);
    void on_connect(sockaddr_in serveraddr) override;
    void on_message(char *data, size_t len) override;
    void on_close() override;
};
} // namespace client
} // namespace ti
