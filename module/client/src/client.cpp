#include "client.h"
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

    socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
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
        while (running) {
            sockmtx.lock();
            char *tres = (char *)calloc(1, sizeof(char));
            ssize_t n = recv(socketfd, tres, 1, 0);
            if (n <= 0) {
                break;
            }

            char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
            n = recv(socketfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
            if (n < 0) {
                break;
            }
            size_t msize = read_len_header(tsize);
            char *buff;
            if (msize > 0) {
                buff = (char *)calloc(msize, sizeof(char));
                n = recv(socketfd, buff, msize, 0);
            } else {
                buff = nullptr;
                n = 0;
            }
            if (n < 0) {
                break;
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
        sockmtx.unlock();
        on_close();
        running = false;
    }).detach();
}
void Client::stop() {
    if (!running) {
        throw std::runtime_error("client is not running");
    }
    running = false;
    ::closesocketfd(socketfd);
}
Response Client::send(const RequestCode req_c, const void *data, size_t len) {
    if (!running) {
        throw std::runtime_error("client not running");
    }
    auto *treq = (char *)calloc(1, sizeof(char));
    treq[0] = req_c;
    char *tsize = write_len_header(len);
    ::send(socketfd, treq, sizeof(char), 0);
    ::send(socketfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
    ::send(socketfd, data, len, 0);
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
    auto res = send(req_c, buf, sizeof buf);
    delete buf;
    return res;
}
