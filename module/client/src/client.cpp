#include "client.h"
#include <helper.h>
#include <log.h>
#include <thread>

using namespace ti::client;

Client::Client(std::string addr, short port)
    : addr(std::move(addr)), port(port), running(false) {}
Client::~Client() { ::closesocketfd(socketfd); }
bool Client::is_running() const { return running; }
void Client::start() {
    if (running) {
        throw std::runtime_error("client already running");
    }

#if _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("failed to initialize winsock");
    }
    socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketfd == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("failed to create socket");
    }
#else
    socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
#endif
    struct sockaddr_in serveraddr;
    std::memset(&serveraddr, 0, sizeof serveraddr);
    serveraddr.sin_addr.s_addr = inet_addr(addr.c_str());
    serveraddr.sin_family = PF_INET;
    serveraddr.sin_port = htons(port);
    if (::connect(socketfd, (struct sockaddr *)&serveraddr, sizeof serveraddr) <
        0) {
        throw std::runtime_error("failed to connect");
    }
    running = true;
    on_connect(serveraddr);

    std::thread([&] {
        detmtx.lock();

        while (running) {
            sockmtx.lock();
            char *tres = (char *)calloc(1, sizeof(char));
            size_t n = recv(socketfd, tres, 1, 0);
            if (n <= 0) {
                break;
            }

            char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
            n = recv(socketfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
            if (n <= 0) {
                break;
            }
            size_t msize = ti::helper::read_len_header(tsize);
            char *buff;
            if (msize > 0) {
                buff = (char *)calloc(msize, sizeof(char));
                n = recv(socketfd, buff, msize, 0);
                if (n <= 0) {
                    break;
                }
            } else {
                buff = nullptr;
                n = 0;
            }
            auto res_c = (ResponseCode)*tres;
            if (res_c == ResponseCode::MESSAGE) {
                on_message(buff, n);
                delete buff;
            } else {
                resmtx.lock();
                res_queue.push(Response{buff, n, res_c});
                resmtx.unlock();
            }
            delete tres;

            sockmtx.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        on_close();
        running = false;
        detmtx.unlock();
    }).detach();
}
void Client::stop() {
    if (!running) {
        throw std::runtime_error("client is not running");
    }
    running = false;
    ::closesocketfd(socketfd);
    detmtx.lock();
    detmtx.unlock();
}
Response Client::send(const RequestCode req_c, const void *data, size_t len) {
    if (!running) {
        throw std::runtime_error("client not running");
    }
    auto *treq = (char *)calloc(1, sizeof(char));
    treq[0] = req_c;
    char *tsize = ti::helper::write_len_header(len);
    compat::socket::send(socketfd, treq, sizeof(char), 0);
    compat::socket::send(socketfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
    compat::socket::send(socketfd, data, len, 0);
    delete tsize;
    delete treq;

    while (res_queue.empty()) {
        // sockmtx should be unlocked after receiving
        sockmtx.lock();
        sockmtx.unlock();
        if (!running) {
            throw std::runtime_error("connection closed unexpectedly");
        }
    }
    resmtx.lock();
    auto front = res_queue.front();
    res_queue.pop();
    resmtx.unlock();
    return front;
}

Response Client::send(ti::RequestCode req_c, const std::string &content) {
    char *buf = (char *)calloc(req_len(content), sizeof(char));
    append_req_buffer(buf, content);
    auto res = send(req_c, buf, content.length());
    delete buf;
    return res;
}
