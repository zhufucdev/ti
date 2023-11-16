#include "protocol.hpp"

namespace ti {
namespace server {
class Contact {
    User *owner;
    Entity *contact;

  public:
    Contact(User *owner, Entity *contact);
    Entity *get_owner() const;
    Entity *get_contact() const;
};
class ServerOrm : public orm::TiOrm {
    std::vector<Contact *> contacts;

  public:
    explicit ServerOrm(const std::string &dbfile);
    const std::vector<Contact *> &get_contacts();
    std::vector<Entity *> get_contacts(User *owner);
};
class TiServer : public Server {
    const ServerOrm db;

  public:
    TiServer(std::string addr, short port, std::string dbfile);
    ~TiServer();
    Client *on_connect(sockaddr_in addr) override;
};
class TiClient : public Client {
    const ServerOrm &db;
    std::string id;

  public:
    explicit TiClient(const ServerOrm &db);
    ~TiClient() override;
    void on_connect(sockaddr_in addr) override;
    void on_disconnect() override;
    void on_message(char *data, size_t len) override;
};
} // namespace server
} // namespace ti
