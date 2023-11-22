#include <gtest/gtest.h>
#include <nanoid.h>
#include <ti_client.h>

using namespace ti;

class ClientTest : public testing::Test {
  protected:
    client::TiClient *client{};
    std::string dbfile = nanoid::generate() + ".db";
    std::string user_name = "testificate_man", password = "root";
    void SetUp() override {
        client = new client::TiClient("127.0.0.1", 6789, dbfile);
        client->start();
    }

    void TearDown() override {
        client->stop();
        delete client;
        std::remove(dbfile.c_str());
    }
};

TEST_F(ClientTest, Login) {
    auto id = client->user_reg(user_name, password);
    ASSERT_TRUE(client->user_login(id, password));
    ASSERT_TRUE(client->user_delete());
}