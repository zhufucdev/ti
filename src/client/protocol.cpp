#include <client/protocol.hpp>

using namespace ti::client;

Client::Client(std::string addr, short port)
    : addr(std::move(addr)), port(port), running(false) {}
Client::~Client() { ::closesocketfd(socketfd); }
bool Client::is_running() const { return running; }
void Client::start() {
    if (running) {
        throw std::runtime_error("client already running");
    }

    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in serveraddr;
    std::memset(&serveraddr, 0, sizeof serveraddr);
    serveraddr.sin_addr.s_addr = inet_addr(addr.c_str());
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    if (::connect(socketfd, (struct sockaddr *)&serveraddr, sizeof serveraddr) <
        0) {
        throw std::runtime_error("failed to connect");
    }
    running = true;
    on_connect(serveraddr);

    std::thread([&] {
        while (running) {
            char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
            int n = recv(socketfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
            if (n <= 0) {
                break;
            }
            size_t msize = read_len_header(tsize);
            char *buff = (char *)calloc(msize, sizeof(char));
            n = recv(socketfd, buff, msize, 0);
            if (n <= 0) {
                break;
            }
            on_message(buff, n);
        }
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
void Client::send(const void *data, size_t len) {
    char *tsize = write_len_header(len);
    ::send(socketfd, tsize, BYTES_LEN_HEADER * sizeof(char), 0);
    ::send(socketfd, data, len, 0);
}
