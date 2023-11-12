#include "protocol.hpp"
#include "sqlite3.h"

namespace ti {
namespace server {
class TiServer : public Server {
    sqlite3 *dbhandle;

  public:
    TiServer(std::string addr, short port, std::string dbfile);
    ~TiServer();
    Client *on_connect(sockaddr_in addr) override;
};
class TiClient : public Client {
    sqlite3 *dbhandle;
    std::string id;

  public:
    explicit TiClient(sqlite3 *dbhandle);
    ~TiClient() override;
    void on_connect(sockaddr_in addr) override;
    void on_disconnect() override;
    void on_message(char *content, size_t len) override;
};
} // namespace server
} // namespace ti
