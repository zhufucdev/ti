#include <server/behavior.hpp>
#include <log.hpp>

using namespace ti::server;

ServerOrm::ServerOrm(const std::string &dbfile) : TiOrm(dbfile) {
    exec_sql_no_result("CREATE TABLE IF NOT EXISTS \"password\"(\n    \n) ");
}
const std::vector<Contact *> &ServerOrm::get_contacts() { return contacts; }
std::vector<ti::Entity *> ServerOrm::get_contacts(User *owner) {
    std::vector<Entity *> ev;
    for (auto e : contacts) {
        if (e->get_owner() == owner) {
            ev.push_back(e->get_contact());
        }
    }
    return ev;
}

Contact::Contact(User *owner, Entity *contact)
    : owner(owner), contact(contact) {}
ti::Entity *Contact::get_contact() const { return contact; }
ti::Entity *Contact::get_owner() const { return owner; }

TiServer::TiServer(std::string addr, short port, std::string dbfile)
    : Server(std::move(addr), port), db(dbfile) {}
TiServer::~TiServer() = default;
Client *TiServer::on_connect(sockaddr_in addr) { return new TiClient(db); }

std::vector<std::string> read_message_body(const char *data, size_t len) {
    int i = 1, j = 1;
    std::vector<std::string> heap;
    while (i < len) {
        if (data[i] == '\0') {
            std::string substr(data + j, i - j);
            heap.push_back(substr);
            j = i + 1;
        }
        i++;
    }
    if (j != i) {
        std::string substr(data + j, len);
        heap.push_back(substr);
    }
    return heap;
}

TiClient::TiClient(const ServerOrm &db) : db(db) {}
TiClient::~TiClient() = default;
void TiClient::on_connect(sockaddr_in addr) {
    logD("Connected to %s as %s", inet_ntoa(addr.sin_addr), id.c_str());
}
void TiClient::on_message(char *data, size_t len) {
    auto req_t = (RequestType)data[0];
    auto body = read_message_body(data, len);
    switch (req_t) {
    case LOGIN:

        break;
    }
    send(data, len);
}

void TiClient::on_disconnect() { logD("%s disconnected", id.c_str()); }
