#include "../protocol.hpp"
#include "sqlite3.h"

namespace ti {
namespace client {
class Client {
    std::string addr;
    short port;
    SocketFd socketfd;
    bool running;

  public:
    Client(std::string addr, short port);
    ~Client();
    void start();
    void stop();
    void send(const void *data, size_t len);
    bool is_running() const;
    virtual void on_connect(sockaddr_in serveraddr) = 0;
    virtual void on_message(char *data, size_t len) = 0;
    virtual void on_close() = 0;
};
} // namespace client
} // namespace ti