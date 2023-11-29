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

class DueClientTest : public ClientTest {
  protected:
    client::TiClient *client2{};
    std::string dbfile2 = nanoid::generate() + ".db";
    void SetUp() override {
        ClientTest::SetUp();
        client2 = new client::TiClient("127.0.0.1", 6789, dbfile2);
    }

    void TearDown() override {
        client2->stop();
        delete client2;
        std::remove(dbfile2.c_str());
    }
};

TEST_F(ClientTest, Login) {
    auto id = client->user_reg(user_name, password);
    ASSERT_TRUE(client->user_login(id, password));
    ASSERT_TRUE(client->user_delete());
}

TEST_F(ClientTest, Sync) {

}