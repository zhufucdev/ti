#include "../ti.hpp"

#ifdef _WIN32
#define SocketFd SOCKET
#else
#define SocketFd int
#endif

namespace ti {
class TiServer {
    bool running;
    std::string addr;
    short port;
    SocketFd socketfd;

  public:
    TiServer(std::string addr, short port) : addr(addr), port(port){};

    std::string get_addr();
    short get_port();

    void start();
    void stop();
    void handleconn(SocketFd clientfd);
};
} // namespace ti
