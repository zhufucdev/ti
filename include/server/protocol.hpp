#include "../protocol.hpp"
#include "sqlite3.h"

#define BUFFER_SIZE 512
#define SendFn std::function<void(char *, int)>

namespace ti {
namespace server {
class Client {
    SendFn sendfn;

  public:
    virtual ~Client() = default;
    void initialize(SendFn fn);
    void send(char *content, int len) const;
    virtual void on_connect(sockaddr_in addr) = 0;
    virtual void on_message(char *content, int len) = 0;
    virtual void on_disconnect() = 0;
};
class Server {
    bool running;
    std::string addr;
    short port;
    SocketFd socketfd;
    void handleconn(sockaddr_in addr, SocketFd clientfd);

  public:
    Server(std::string addr, short port);
    ~Server();
    void send(SocketFd clientfd, char *data, int len);
    virtual Client *on_connect(sockaddr_in addr) = 0;
    void start();
    void stop();
    std::string get_addr();
    short get_port();
};
} // namespace server
} // namespace ti