#include <gtest/gtest.h>
#include <ti.h>

TEST(Time, Stringify) {
    ASSERT_EQ(ti::to_iso_time(1318057629), "2011-10-08T07:07:09Z");
}

TEST(Time, Parse) {
    ASSERT_EQ(ti::parse_iso_time("2011-10-08T07:07:09Z"), 1318057629);
}