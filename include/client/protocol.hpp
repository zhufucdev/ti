#include "../protocol.hpp"
#include "sqlite3.h"

namespace ti {
namespace client {
class Client {
    std::string addr;
    short port;
    SocketFd socketfd;

  public:
    Client(std::string addr, short port);
    ~Client();
    void start();
    void stop();
    void send(const void *data, size_t len);
    virtual void on_message() = 0;
};


} // namespace client
} // namespace ti