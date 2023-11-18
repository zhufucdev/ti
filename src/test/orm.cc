#include "nanoid.h"
#include "ti_server.h"
#include <gtest/gtest.h>
#include <thread>

class ServerOrmTest : public testing::Test {
  protected:
    ti::server::ServerOrm *sorm{};
    const std::string dbfile = "ti_server.db";
    const std::string user_name = "Testificate Man";
    const std::string user_id = "l1mITy-T1UBWsGeqLszsL";
    const std::string bio = "I test a lot";
    const std::string group_name = "Sick Group";
    const std::string group_id = "C4E7tI86DrbVJdBMphpV0";

    void SetUp() override { sorm = new ti::server::ServerOrm(dbfile); }
    void TearDown() override { delete sorm; }
};

TEST_F(ServerOrmTest, AddUser) {
    sorm->pull();
    sorm->add_user(new ti::User(user_id, user_name, bio), nanoid::generate());

    sorm->pull();

    auto user = sorm->get_user(user_id);
    ASSERT_NE(user, nullptr);
    ASSERT_EQ(user->get_id(), user_id);
    ASSERT_EQ(user->get_name(), user_name);
    ASSERT_EQ(user->get_bio(), bio);
    delete user;
}

TEST_F(ServerOrmTest, GetEntities) {
    sorm->pull();
    auto u = sorm->get_user(user_id);
    ASSERT_NE(u, nullptr);
    ASSERT_EQ(u->get_name(), user_name);
    ASSERT_EQ(u->get_bio(), bio);
    delete u;
}

TEST_F(ServerOrmTest, AddGroup) {
    sorm->pull();
    auto u =
        new ti::User(nanoid::generate(), "Testificate Woman", "I test a lot");
    auto g =
        new ti::Group(group_id, group_name,
                      std::vector<ti::Entity *>{sorm->get_user(user_id), u});

    sorm->add_entity(g);

    sorm->pull();
    auto e = sorm->get_entity(group_id);
    ASSERT_NE(e, nullptr);
    auto gb = dynamic_cast<ti::Group *>(e);
    ASSERT_NE(gb, nullptr);
    ASSERT_EQ(gb->get_name(), group_name);
    ASSERT_EQ(gb->get_members().size(), 2);
    ASSERT_EQ(gb->get_members()[0]->get_id(), user_id);
    ASSERT_EQ(gb->get_members()[1]->get_id(), u->get_id());
}
