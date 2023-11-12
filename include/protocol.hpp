#include "nanoid/nanoid.h"
#include <iostream>
#include <vector>

#define BYTES_LEN_HEADER 8

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define closesocketfd(A)                                                       \
    closesocket(A);                                                            \
    WSACleanup()
#define SocketFd SOCKET
#else
#define closesocketfd(A) close(A)
#define SocketFd int
#endif

namespace ti {
const std::string version = "0.1";

char *write_len_header(size_t len);
size_t read_len_header(char *tsize);

class Entity {
  public:
    virtual std::string get_id() = 0;
};

class Server : Entity {
  public:
    std::string get_id() override;
    Server();
};

class User : Entity {
    std::string name, id;
  public:
    User(std::string id, std::string name);
    explicit User(std::string name);

    std::string get_id() override;
    std::string get_name();
};

class Frame {
  public:
    virtual std::string to_string() = 0;
    virtual std::string get_id() = 0;
};

class TextFrame : Frame {
    std::string content;
    std::string id;

  public:
    TextFrame(std::string id, std::string content);
    explicit TextFrame(std::string content);
    std::string get_id() override;
    std::string to_string() override;
};

class Message {
    std::vector<Frame *> content;
    std::string id;

  public:
    explicit Message(std::vector<Frame *> content);
    Message(std::string id, std::vector<Frame *> content);
    std::vector<Frame *> get_content();
    std::string get_id();
};
} // namespace ti
