#include <gtest/gtest.h>
#include <nanoid.h>
#include <ti_server.h>

class PasswordTest : public testing::Test {
  protected:
    std::string dbfile = nanoid::generate() + ".db";
    ti::server::ServerOrm *sorm = new ti::server::ServerOrm(dbfile);
    const std::string passcode = "root";
    ti::User *user = new ti::User(nanoid::generate(), "", "", 0);

    void SetUp() override { sorm->add_user(user, passcode); }
    void TearDown() override {
        delete sorm;
        std::remove(dbfile.c_str());
    }
};

TEST_F(PasswordTest, Hashing) {
    ASSERT_TRUE(sorm->check_password(user->get_id(), passcode));
    ASSERT_FALSE(sorm->check_password(user->get_id(), "toor"));
}