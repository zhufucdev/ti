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
    void connect();
    void close();
    void send(const void *data, size_t len);
    virtual void on_message()
};


} // namespace client
} // namespace ti