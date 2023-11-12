#include "nanoid/nanoid.h"
#include <iostream>
#include <vector>

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

class Text : Frame {
    std::string content;
    std::string id;

  public:
    Text(std::string id, std::string content);
    explicit Text(std::string content);
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
