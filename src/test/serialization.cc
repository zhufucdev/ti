#include <gtest/gtest.h>
#include <nanoid.h>
#include <ti.h>

using namespace ti;
using namespace std;

class SerializationTest : public testing::Test {
  protected:
    User *user = nullptr;
    Group *group = nullptr;
    TextFrame *tf = nullptr;
    vector<Entity *> entities;
    vector<Frame *> frames;

    void SetUp() override {
        std::time_t now;
        std::time(&now);

        for (int i = 0; i < 114; ++i) {
            entities.push_back(new User(nanoid::generate(),
                                        "Guy " + nanoid::generate(4),
                                        "I am a random guy", now));
        }

        user = new User(nanoid::generate(), "Testificate Man",
                        "I test like superhuman", now);
        group = new Group(nanoid::generate(), "Testificate Group", {user});

        entities.push_back(user);
        entities.push_back(group);

        for (int i = 0; i < 514; ++i) {
            frames.push_back(new TextFrame(nanoid::generate(),
                                           "I mean " + nanoid::generate(4) +
                                               " is much better than " +
                                               nanoid::generate(4)));
        }
        tf = new TextFrame(nanoid::generate(), "Bala ping Bala Pong");
        frames.push_back(tf);
    }

    void TearDown() override {
        for (auto ptr : entities) {
            delete ptr;
        }
        for (auto ptr : frames) {
            delete ptr;
        }
    }
};

TEST_F(SerializationTest, User) {
    char *bs;
    auto len = user->serialize(&bs);

    auto su = User::deserialize(bs, len);
    ASSERT_EQ(su->get_id(), user->get_id());
    ASSERT_EQ(su->get_name(), user->get_name());
    ASSERT_EQ(su->get_bio(), user->get_bio());
    ASSERT_EQ(su->get_registration_time(), user->get_registration_time());
    delete su;
    delete bs;
}

TEST_F(SerializationTest, Group) {
    char *bs;
    auto len = group->serialize(&bs);

    auto sg = Group::deserialize(bs, len, entities);
    ASSERT_EQ(sg->get_id(), group->get_id());
    ASSERT_EQ(sg->get_name(), group->get_name());
    ASSERT_EQ(sg->get_members(), vector<Entity *>{user});

    delete bs;
    delete sg;
}

TEST_F(SerializationTest, TextFrame) {
    char *bs;
    auto len = tf->serialize(&bs);

    auto stf = TextFrame::deserialize(bs, len);
    ASSERT_EQ(stf->get_id(), tf->get_id());
    ASSERT_EQ(stf->to_string(), tf->to_string());

    delete bs;
    delete stf;
}

TEST_F(SerializationTest, Message) {
    char *bs;
    time_t now;
    time(&now);

    Message msg(nanoid::generate(), {tf}, now, user, group, nullptr),
        msg_f(nanoid::generate(), {tf}, now, user, user, group);

    auto len = msg.serialize(&bs);
    auto smsg = Message::deserialize(bs, len, frames, entities);
    ASSERT_EQ(smsg->get_id(), msg.get_id());
    ASSERT_EQ(smsg->get_frames(), msg.get_frames());
    ASSERT_EQ(smsg->get_sender(), msg.get_sender());
    ASSERT_EQ(smsg->get_receiver(), msg.get_receiver());
    ASSERT_EQ(smsg->get_forward_source(), msg.get_forward_source());
    delete smsg;

    delete bs;
    len = msg_f.serialize(&bs);
    auto smsg_f = Message::deserialize(bs, len, frames, entities);
    ASSERT_EQ(smsg_f->get_id(), msg_f.get_id());
    ASSERT_EQ(smsg_f->get_frames(), msg_f.get_frames());
    ASSERT_EQ(smsg_f->get_sender(), msg_f.get_sender());
    ASSERT_EQ(smsg_f->get_receiver(), msg_f.get_receiver());
    ASSERT_EQ(smsg_f->get_forward_source(), msg_f.get_forward_source());
    delete smsg_f;
    delete bs;
}