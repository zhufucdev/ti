#include <gtest/gtest.h>
#include <helper.h>
#include <nanoid.h>

using namespace ti::helper;

TEST(Diff, Diff) {
    std::vector<std::string> common, plus, minus;
    for (int i = 0; i < 5; ++i) {
        common.push_back(nanoid::generate());
    }
    for (int i = 0; i < 5; ++i) {
        plus.push_back(nanoid::generate());
    }
    for (int i = 0; i < 5; ++i) {
        minus.push_back(nanoid::generate());
    }
    std::vector<std::string> a(common), b(common);
    a.insert(a.end(), plus.begin(), plus.end());
    b.insert(b.end(), minus.begin(), minus.end());
    Diff<std::string> diff(a.begin(), a.end(), b.begin(), b.end());
    ASSERT_EQ(diff.plus, plus);
    ASSERT_EQ(diff.minus, minus);
}