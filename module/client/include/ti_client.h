#include "client.h"

namespace ti {
namespace client {

enum TiClientState { OFFLINE, LOGGED_OUT, READY, SENDING };

class TiClient : public Client, public orm::TiOrm {
    TiClientState state;
    sqlite3 *dbhandle;
    std::string userid, token;

  public:
    TiClient(std::string addr, short port, const std::string& dbfile);
    ~TiClient();
    TiClientState get_state();
    bool user_login(const std::string&user_id, const std::string& password);
    std::string user_reg(const std::string& name, const std::string& password);
    bool user_logout();
    std::vector<User *> get_current_user() const;
    std::vector<Entity *> get_contacts() const;
    void send(Message *message);
    void on_connect(sockaddr_in serveraddr) override;
    void on_message(char *data, size_t len) override;
    void on_close() override;
};
} // namespace client
} // namespace ti
