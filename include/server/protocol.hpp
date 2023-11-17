#include "../protocol.hpp"

#define SendFn std::function<void(ti::ResponseCode, char *, size_t)>

namespace ti {
namespace server {
class Client {
    SendFn sendfn;

  public:
    virtual ~Client() = default;
    void initialize(SendFn fn);
    void send(ResponseCode res, char *content, size_t len) const;
    void send(ResponseCode res) const;
    virtual void on_connect(sockaddr_in addr) = 0;
    virtual void on_message(RequestCode req, char *content, size_t len) = 0;
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
    static void send(SocketFd clientfd, ResponseCode res, char *data, size_t len);
    virtual Client *on_connect(sockaddr_in addr) = 0;
    void start();
    void stop();
    std::string get_addr() const;
    short get_port() const;
    bool is_running() const;
};
} // namespace server
} // namespace ti
