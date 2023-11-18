#include "log.h"
#include "ti_server.h"
#include <csignal>
#include <iostream>

using ti::server::TiServer;

static TiServer *server;

void interrupt(int param);

int main(int argc, char const *argv[]) {
    std::cout<<"TI - The IM server version "<<ti::version<<std::endl;
    logD("Running a debug version");

    signal(SIGINT, interrupt);
    ti::orm::SqlDatabase::initialize();

    server = new TiServer("0.0.0.0", 6789, "ti_server.db");
    std::cout<<"Listen on "<<server->get_addr()<<":"<<server->get_port()<<std::endl;
    server->start();
    delete server;
    ti::orm::SqlDatabase::shutdown();
    return 0;
}

void interrupt(int) {
    if (server->is_running()) {
        server->stop();
    }
}