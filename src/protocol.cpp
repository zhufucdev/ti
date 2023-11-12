#include "protocol.hpp"

using namespace ti;

char *ti::write_len_header(size_t len) {
    char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
    int n = BYTES_LEN_HEADER - sizeof(len);
    while (n < BYTES_LEN_HEADER) {
        tsize[n] = (len >> (sizeof(len) - n - 1) * 8);
        n++;
    }
    return tsize;
}

size_t ti::read_len_header(char *tsize) {
    int n = 0;
    size_t msize = 0;
    while (n < BYTES_LEN_HEADER) {
        msize <<= sizeof(char);
        msize |= tsize[n++];
    }
    return msize;
}

Server::Server() = default;
std::string Server::get_id() { return "zGuEzyj3EUyeSKAvHw3Zo"; }

User::User(std::string name) : name(std::move(name)), id(nanoid::generate()) {}
User::User(std::string id, std::string name)
    : id(std::move(id)), name(std::move(name)) {}
std::string User::get_id() { return id; }
std::string User::get_name() { return name; }

TextFrame::TextFrame(std::string content)
    : content(std::move(content)), id(nanoid::generate()) {}
TextFrame::TextFrame(std::string id, std::string content)
    : id(std::move(id)), content(std::move(content)) {}
std::string TextFrame::get_id() { return id; }
std::string TextFrame::to_string() { return content; }

Message::Message(std::vector<Frame *> content) : content(std::move(content)) {}
Message::Message(std::string id, std::vector<Frame *> content)
    : id(std::move(id)), content(std::move(content)) {}
std::vector<Frame *> Message::get_content() { return content; }
std::string Message::get_id() { return id; }
