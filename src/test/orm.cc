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
    sorm->add_entity(new ti::User(testificate_man));
    sorm->pull();

    auto user = sorm->get_user(testificate_man.get_id());
    ASSERT_NE(user, nullptr);
    ASSERT_EQ(user->get_id(), testificate_man.get_id());
    ASSERT_EQ(user->get_name(), testificate_man.get_name());
    ASSERT_EQ(user->get_bio(), testificate_man.get_bio());
}

TEST_F(ServerOrmTest, GetEntities) {
    sorm->add_entity(new ti::User(testificate_man));
    sorm->pull();

    auto u = sorm->get_user(testificate_man.get_id());
    ASSERT_NE(u, nullptr);
    ASSERT_EQ(u->get_name(), testificate_man.get_name());
    ASSERT_EQ(u->get_bio(), testificate_man.get_bio());
}

TEST_F(ServerOrmTest, AddGroup) {
    sorm->add_entity(new ti::User(testificate_man));
    sorm->add_entity(new ti::User(testificate_woman));
    sorm->add_entity(new ti::Group(group));

    sorm->pull();
    auto e = sorm->get_entity(group.get_id());
    ASSERT_NE(e, nullptr);
    auto gb = dynamic_cast<ti::Group *>(e);
    ASSERT_NE(gb, nullptr);
    ASSERT_EQ(gb->get_name(), group.get_name());
    ASSERT_EQ(gb->get_members().size(), 2);
    ASSERT_EQ(gb->get_members()[0]->get_id(), testificate_man.get_id());
    ASSERT_EQ(gb->get_members()[1]->get_id(), testificate_woman.get_id());
}

TEST_F(ServerOrmTest, AddMessage) {
    time_t now;
    time(&now);
    auto tm = new ti::User(testificate_man),
         tw = new ti::User(testificate_woman);
    sorm->add_entity(tm);
    sorm->add_entity(tw);
    ti::TextFrame tfs[] = {
        ti::TextFrame(nanoid::generate(), "Hi I'm John Cena"),
        ti::TextFrame(nanoid::generate(), "I like bing chilling"),
        ti::TextFrame(nanoid::generate(), nanoid::generate(1 << 12))};
    std::vector<ti::Frame *> frames = {new ti::TextFrame(tfs[0]),
                                       new ti::TextFrame(tfs[1]),
                                       new ti::TextFrame(tfs[2])};
    auto msg = ti::Message(nanoid::generate(), frames, now, tm, tw, nullptr);
    sorm->add_message(new ti::Message(msg));

    sorm->pull();
    auto m = sorm->get_message(msg.get_id());
    ASSERT_EQ(m->get_time(), now);
    ASSERT_EQ(m->get_frames().size(), frames.size());
    ASSERT_EQ(m->get_frames()[0]->get_id(), tfs[0].get_id());
    ASSERT_EQ(m->get_frames()[0]->to_string(), tfs[0].to_string());
    ASSERT_EQ(m->get_frames()[2]->to_string(), tfs[2].to_string());
}

TEST_F(ServerOrmTest, AddContact) {
    auto tm = new ti::User(testificate_man),
         tw = new ti::User(testificate_woman);
    auto g = new ti::Group(group);
    sorm->add_entity(tm);
    sorm->add_entity(tw);
    sorm->add_entity(g);
    sorm->add_contact(tm, tw);
    sorm->add_contact(tm, g);
    sorm->pull();

    auto contacts =
        sorm->get_contacts(sorm->get_user(testificate_man.get_id()));
    ASSERT_EQ(contacts[0]->get_id(), testificate_woman.get_id());
    ASSERT_EQ(contacts[1]->get_id(), group.get_id());
}