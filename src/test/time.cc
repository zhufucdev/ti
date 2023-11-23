#include <gtest/gtest.h>
#include <ti.h>

TEST(Time, Stringify) {
    ASSERT_EQ(ti::to_iso_time(1318057629), "2011-10-08T07:07:09Z");
}

TEST(Time, Parse) {
    ASSERT_EQ(ti::parse_iso_time("2011-10-08T07:07:09Z"), 1318057629);
}

class TimeSqlTest : public testing::Test {
  protected:
    time_t now;
    ti::orm::SqlDatabase *db;
    std::string dbfile = "time_sql_test.db";
    void SetUp() override {
        std::time(&now);
        ti::orm::SqlDatabase::initialize();
        db = new ti::orm::SqlDatabase(dbfile);
    }
    void TearDown() override {
        delete db;
        std::remove(dbfile.c_str());
    }
};

TEST_F(TimeSqlTest, Sql) {
    db->exec_sql(R"(CREATE TABLE "time"
(
    id    int primary key,
    value datetime not null
))");
    auto t = db->prepare(R"(INSERT INTO "time"(value) VALUES (?))");
    auto tstr = ti::to_iso_time(now);
    t->bind_text(0, tstr);
    t->begin();
    delete t;

    t = db->prepare(R"(SELECT value FROM "time")");
    auto row = *(t->begin());
    ASSERT_EQ(ti::parse_iso_time(row.get_text(0)), now);
}