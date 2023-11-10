#include <iostream>
#include <server/ti.hpp>
#include <csignal>

static ti::TiServer *server;

void interrupt(int param);

int main(int argc, char const *argv[]) {
    std::cout<<"TI - The IM server version "<<ti::version<<std::endl;
    
    signal(SIGINT, interrupt);

    server = new ti::TiServer("0.0.0.0", 6789);
    std::cout<<"listen on "<<server->get_addr()<<":"<<server->get_port()<<std::endl;
    server->start();
    return 0;
}

void interrupt(int param) {
    server->stop();
}