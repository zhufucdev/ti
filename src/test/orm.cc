#include "nanoid.h"
#include "ti_server.h"
#include <gtest/gtest.h>
#include <thread>

class ServerOrmTest : public testing::Test {
  protected:
    const std::string dbfile = nanoid::generate() + ".db";
    ti::server::ServerOrm *sorm{};
    const std::string group_name = "Sick Group";
    const std::string group_id = "C4E7tI86DrbVJdBMphpV0";
    ti::User testificate_man{"l1mITy-T1UBWsGeqLszsL", "Testificate Man",
                             "I test a lot"};
    ti::User testificate_woman{"Z0RSddx7esE8lmT0fZ1Yc", "Testificate Woman",
                               "I test a lot"};

    void SetUp() override { sorm = new ti::server::ServerOrm(dbfile); }
    void TearDown() override {
        delete sorm;
        std::remove(dbfile.c_str());
    }
};

TEST_F(ServerOrmTest, AddUser) {
    sorm->add_user(&testificate_man, nanoid::generate());
    sorm->pull();

    auto user = sorm->get_user(testificate_man.get_id());
    ASSERT_NE(user, nullptr);
    ASSERT_EQ(user->get_id(), testificate_man.get_id());
    ASSERT_EQ(user->get_name(), testificate_man.get_name());
    ASSERT_EQ(user->get_bio(), testificate_man.get_bio());
    delete user;
}

TEST_F(ServerOrmTest, GetEntities) {
    sorm->add_user(&testificate_man, nanoid::generate());
    sorm->pull();

    auto u = sorm->get_user(testificate_man.get_id());
    ASSERT_NE(u, nullptr);
    ASSERT_EQ(u->get_name(), testificate_man.get_name());
    ASSERT_EQ(u->get_bio(), testificate_man.get_bio());
    delete u;
}

TEST_F(ServerOrmTest, AddGroup) {
    sorm->add_user(&testificate_man, nanoid::generate());
    sorm->add_user(&testificate_woman, nanoid::generate());
    auto g = new ti::Group(
        group_id, group_name,
        std::vector<ti::Entity *>{&testificate_man, &testificate_woman});
    sorm->add_entity(g);

    sorm->pull();
    auto e = sorm->get_entity(group_id);
    ASSERT_NE(e, nullptr);
    auto gb = dynamic_cast<ti::Group *>(e);
    ASSERT_NE(gb, nullptr);
    ASSERT_EQ(gb->get_name(), group_name);
    ASSERT_EQ(gb->get_members().size(), 2);
    ASSERT_EQ(gb->get_members()[0]->get_id(), testificate_man.get_id());
    ASSERT_EQ(gb->get_members()[1]->get_id(), testificate_woman.get_id());
    delete g;
    delete gb;
}
