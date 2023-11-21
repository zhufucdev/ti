#include <gtest/gtest.h>
#include <nanoid.h>
#include <ti_client.h>

using namespace ti;

class NetworkingTest : public testing::Test {
  protected:
    client::TiClient *client{};
    std::string dbfile = nanoid::generate() + ".db";
    std::string user_name = "testificate_man", password = "root";
    std::string user_id = "4Q-9BLD4UNDJvnGdaGtq-";
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

TEST_F(NetworkingTest, Login) {
    auto logged_in = client->user_login(user_id, password);
    if (!logged_in) {
        ASSERT_TRUE(client->user_reg(user_name, password));
    }
    logged_in = client->user_login(user_id, password);
    ASSERT_TRUE(logged_in);
}