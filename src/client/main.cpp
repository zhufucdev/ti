#include <client/behavior.hpp>
#include <log.hpp>

using ti::client::TiClient;

static TiClient *client;

void interrupt(int param);

int main(int argc, char *argv[]) {
    std::cout << "TI - The IM client version " << ti::version << std::endl;
    logD("Running a debugging version");

    signal(SIGINT, interrupt);
    ti::orm::SqlHelper::initialize();

    client = new TiClient("127.0.0.1", 6789, "ti_client.db");
    client->start();

    char cmd;
    while (client->is_running()) {
        std::cout << "ti> ";
        std::cin >> cmd;

        if (cmd == 'q') {
            break;
        }
    }

    if (client->is_running()) {
        client->stop();
    }
    delete client;
    ti::orm::SqlHelper::shutdown();
    return 0;
}

void interrupt(int) {
    if (client->is_running()) {
        client->stop();
    }
}