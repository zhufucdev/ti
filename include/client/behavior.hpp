#include "protocol.hpp"

namespace ti {
namespace client {
enum VerificationResult { OK, MISMATCH, NOT_EXISTING };

enum TiClientState { OFFLINE, LOGGED_OUT, READY, SENDING };

class TiClient : public Client {
    TiClientState state;
    sqlite3 *dbhandle;
    std::string userid;

  public:
    TiClient(std::string addr, short port, std::string dbfile);
    ~TiClient();
    VerificationResult user_login(std::string name, std::string password);
    bool user_reg(std::string name, std::string password);
    bool user_logout();
    std::vector<User *> get_current_user();
    std::vector<Entity *> get_contacts();
    Entity *get_entity(std::string id);
    void send(Message *message);
};
} // namespace client
} // namespace ti
