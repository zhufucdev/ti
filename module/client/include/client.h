#include "ti.h"
#include <mutex>
#include <queue>

namespace ti {
namespace client {
struct Response {
    const char *buff;
    const size_t len;
    const ResponseCode code;
};

template <typename T> size_t req_len(const T &str) { return str.length() + 1; }

template <typename T, typename... Args>
size_t req_len(const T &first, const Args &...args) {
    return first.length() + 1 + req_len(args...);
}

template <typename T> void append_req_buffer(char *start, const T &content) {
    std::strcpy(start, content.c_str());
}

template <typename T, typename... Args>
void append_req_buffer(char *start, const T &next, const Args &...args) {
    append_req_buffer(start, next);
    append_req_buffer(start + next.length() + 1, args...);
}

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
    Response send(RequestCode req_c, const std::string &content);
    template <typename... Args>
    Response send(RequestCode req_c, const std::string &first,
                  const Args &...args) {
        size_t len = req_len(first, args...);
        char *buf = (char *)calloc(len, sizeof(char));
        append_req_buffer(buf, first, args...);
        auto res = send(req_c, buf, len);
        delete buf;
        return res;
    }
    bool is_running() const;
    virtual void on_connect(sockaddr_in serveraddr) = 0;
    virtual void on_message(char *data, size_t len) = 0;
    virtual void on_close() = 0;
};
} // namespace client
} // namespace ti