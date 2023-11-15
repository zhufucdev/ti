#include "protocol.hpp"

namespace ti {
namespace server {
class TiServer : public Server {
    const orm::TiOrm db;

  public:
    TiServer(std::string addr, short port, std::string dbfile);
    ~TiServer();
    Client *on_connect(sockaddr_in addr) override;
};
class TiClient : public Client {
    const orm::TiOrm &db;
    std::string id;

  public:
    explicit TiClient(const orm::TiOrm &db);
    ~TiClient() override;
    void on_connect(sockaddr_in addr) override;
    void on_disconnect() override;
    void on_message(char *data, size_t len) override;
};
} // namespace server
} // namespace ti
