#include "../protocol.hpp"
#include "sqlite3.h"

namespace ti {
namespace client {
enum VerificationResult { OK, MISMATCH, NOT_EXISTING };

enum TiClientState { OFFLINE, LOGGED_OUT, READY, SENDING };

class Client {
    std::string addr;
    short port;
    SocketFd socketfd;

  public:
    Client(std::string addr, short port);
    ~Client();
    void connect();
    void close();
    void send(const void *data, size_t len);
};

class TiClient : public Client {
    TiClientState state;
    sqlite3 *dbhandle;
    std::string username;

  public:
    TiClient(std::string addr, short port, std::string dbfile);
    ~TiClient();
    VerificationResult user_login(std::string name, std::string password);
    bool user_reg(std::string name, std::string password);
    bool user_logout();
    std::vector<Sender *> get_contacts();
    void send(Message *message);
};

} // namespace client
} // namespace ti