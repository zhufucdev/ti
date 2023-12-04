#include "nanoid.h"
#include "ti_server.h"
#include <gtest/gtest.h>
#include <thread>

class ServerOrmTest : public testing::Test {
  protected:
    const std::string dbfile = nanoid::generate() + ".db";
    ti::server::ServerOrm *sorm{};
    ti::User testificate_man{"l1mITy-T1UBWsGeqLszsL", "Testificate Man",
                             "I test a lot", 0};
    ti::User testificate_woman{"Z0RSddx7esE8lmT0fZ1Yc", "Testificate Woman",
                               "I test a lot", 0};
    ti::Group group{
        "C4E7tI86DrbVJdBMphpV0", "Sick Group",
        std::vector<ti::Entity *>{&testificate_man, &testificate_woman}};

    void SetUp() override { sorm = new ti::server::ServerOrm(dbfile); }
    void TearDown() override {
        delete sorm;
        std::remove(dbfile.c_str());
    }
};

TEST_F(ServerOrmTest, AddUser) {
    sorm->add_entity(&testificate_man);
    sorm->pull();

    auto user = sorm->get_user(testificate_man.get_id());
    ASSERT_NE(user, nullptr);
    ASSERT_EQ(user->get_id(), testificate_man.get_id());
    ASSERT_EQ(user->get_name(), testificate_man.get_name());
    ASSERT_EQ(user->get_bio(), testificate_man.get_bio());
    delete user;
}

TEST_F(ServerOrmTest, GetEntities) {
    sorm->add_entity(&testificate_man);
    sorm->pull();

    auto u = sorm->get_user(testificate_man.get_id());
    ASSERT_NE(u, nullptr);
    ASSERT_EQ(u->get_name(), testificate_man.get_name());
    ASSERT_EQ(u->get_bio(), testificate_man.get_bio());
    delete u;
}

TEST_F(ServerOrmTest, AddGroup) {
    sorm->add_entity(&testificate_man);
    sorm->add_entity(&testificate_woman);
    sorm->add_entity(&group);

    sorm->pull();
    auto e = sorm->get_entity(group.get_id());
    ASSERT_NE(e, nullptr);
    auto gb = dynamic_cast<ti::Group *>(e);
    ASSERT_NE(gb, nullptr);
    ASSERT_EQ(gb->get_name(), group.get_name());
    ASSERT_EQ(gb->get_members().size(), 2);
    ASSERT_EQ(gb->get_members()[0]->get_id(), testificate_man.get_id());
    ASSERT_EQ(gb->get_members()[1]->get_id(), testificate_woman.get_id());
    delete gb;
}

TEST_F(ServerOrmTest, AddMessage) {
    time_t now;
    time(&now);
    sorm->add_entity(&testificate_man);
    sorm->add_entity(&testificate_woman);
    std::vector<ti::Frame *> frames = {
        new ti::TextFrame(nanoid::generate(), "Hi I'm John Cena"),
        new ti::TextFrame(nanoid::generate(), "I like bing chilling"),
        new ti::TextFrame(nanoid::generate(), nanoid::generate(1 << 12))};
    auto msg = new ti::Message(nanoid::generate(), frames, now,
                               &testificate_man, &testificate_woman, nullptr);
    sorm->add_message(msg);

    sorm->pull();
    auto m = sorm->get_message(msg->get_id());
    ASSERT_EQ(m->get_time(), now);
    ASSERT_EQ(m->get_frames().size(), frames.size());
    ASSERT_EQ(m->get_frames()[0]->get_id(), frames[0]->get_id());
    ASSERT_EQ(m->get_frames()[0]->to_string(), frames[0]->to_string());
    ASSERT_EQ(m->get_frames()[2]->to_string(), frames[2]->to_string());
}

TEST_F(ServerOrmTest, AddContact) {
    sorm->add_entity(&testificate_man);
    sorm->add_entity(&testificate_woman);
    sorm->add_entity(&group);
    sorm->add_contact(&testificate_man, &testificate_woman);
    sorm->add_contact(&testificate_man, &group);
    sorm->pull();

    auto contacts = sorm->get_contacts(&testificate_man);
    ASSERT_EQ(contacts[0]->get_id(), testificate_woman.get_id());
    ASSERT_EQ(contacts[1]->get_id(), group.get_id());
}