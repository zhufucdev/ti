#include "../protocol.hpp"
#include <mutex>
#include <queue>

namespace ti {
namespace client {
struct Response {
    const char *buff;
    const ssize_t len;
    const ResponseCode code;
};


class Client {
    std::string addr;
    short port;
    SocketFd socketfd;
    bool running;
    std::mutex sockmtx, resmtx;
    std::queue<Response> res_queue;

  public:
    Client(std::string addr, short port);
    ~Client();
    void start();
    void stop();
    Response send(RequestCode req_c, const void *data, size_t len);
    bool is_running() const;
    virtual void on_connect(sockaddr_in serveraddr) = 0;
    virtual void on_message(char *data, size_t len) = 0;
    virtual void on_close() = 0;
};
} // namespace client
} // namespace ti