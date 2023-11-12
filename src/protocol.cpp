#include "protocol.hpp"

using namespace ti;

Server::Server() = default;
std::string Server::get_id() { return "zGuEzyj3EUyeSKAvHw3Zo"; }

User::User(std::string name) : name(std::move(name)), id(nanoid::generate()) {}
User::User(std::string id, std::string name)
    : id(std::move(id)), name(std::move(name)) {}
std::string User::get_id() { return id; }
std::string User::get_name() { return name; }

Text::Text(std::string content)
    : content(std::move(content)), id(nanoid::generate()) {}
Text::Text(std::string id, std::string content)
    : id(std::move(id)), content(std::move(content)) {}
std::string Text::get_id() { return id; }
std::string Text::to_string() { return content; }

Message::Message(std::vector<Frame *> content) : content(std::move(content)) {}
Message::Message(std::string id, std::vector<Frame *> content)
    : id(std::move(id)), content(std::move(content)) {}
std::vector<Frame *> Message::get_content() { return content; }
std::string Message::get_id() { return id; }
