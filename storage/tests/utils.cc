
#include <gtest/gtest.h>
#include "storage/storage.h"

using namespace Storage;

static int StringPairCompare(pair<string, string> a, pair<string, string> b);

int StringPairCompare(pair<string, string> a, pair<string, string> b) {
    auto cmp = a.first.compare(b.first);
    if (cmp == 0) {
        return a.second.compare(b.second);
    }
    return cmp;
}

TEST(Comparer, CompareIntegers) {
        EXPECT_EQ(5, Comparer<int>()(10, 5));
}

TEST(Comparer, CompareStrings) {
        string first = "hello";
        string second = "world";
        EXPECT_EQ(first.compare(second), Comparer<string>()(first, second));

        second = "hello";
        EXPECT_EQ(first.compare(second), Comparer<string>()(first, second));
}


TEST(Joiner, JoinStrings) {
        EXPECT_EQ("a/b/c/d", joinStrings({"a", "b", "c", "d"}, "/"));
        EXPECT_EQ("a##b##c##d", joinStrings({"a", "b", "c", "d"}, "##"));
}

TEST(Joiner, NotIgnoreEmptyStrings) {
        EXPECT_EQ("a//c/", joinStrings({"a", "", "c", ""}, "/"));
}

TEST(IterCompare, CompareVectorsEqual) {
        vector<int> v1 = {1,2,3};
        vector<int> v2 = {1,2,3};
        auto result = IterCompare(
                        v1.begin(), v1.end(), v2.begin(), v2.end(),
                        [](int a, int b) {
                            return a - b;
                        });
        EXPECT_EQ(0, result);
}

// Compare Vectors - v1 < v2 should be -1
TEST(IterCompare, CompareVectorsLessThan) {
        vector<int> v1 = {1,2,3};
        vector<int> v2 = {1,2,3,4};
        auto result = IterCompare(
                        v1.begin(), v1.end(), v2.begin(), v2.end(),
                        [](int a, int b) {
                            return a - b;
                        });
        EXPECT_EQ(-1, result);
}

// Compare Vectors - v1 > v2 should be >1
TEST(IterCompare, CompareVectorsGreaterThan) {
        vector<int> v1 = {1,2,10};
        vector<int> v2 = {1,2,3,4};
        auto result = IterCompare(
                        v1.begin(), v1.end(), v2.begin(), v2.end(),
                        [](int a, int b) {
                            return a - b;
                        });
        EXPECT_EQ(true, result > 0);
}

TEST(IterCompare, CompareMapsEqual) {
        map<string, string> m1 = { 
            {"1", "a"},
            {"2", "b"},
            {"3", "c"}
        };
        map<string, string> m2 = { 
            {"1", "a"},
            {"2", "b"},
            {"3", "c"}
        };
        auto result = IterCompare(
                        m1.begin(), m1.end(), m2.begin(), m2.end(), StringPairCompare);
        EXPECT_EQ(0, result);
}

// Compare Maps - m1 < m2 should be -1
TEST(IterCompare, CompareMapsLessThan) {
        map<string, string> m1 = { 
            {"1", "a"},
            {"2", "b"},
            {"3", "c"}
        };
        map<string, string> m2 = { 
            {"1", "a"},
            {"2", "b"},
            {"3", "d"}  // <--- Diff here
        };
        auto result = IterCompare(
                        m1.begin(), m1.end(), m2.begin(), m2.end(),
                        StringPairCompare);
        EXPECT_EQ(-1, result);
    }

// Compare Vectors - v1 > v2 should be >1
TEST(IterCompare, CompareMapsGreaterThan) {
        map<string, string> m1 = { 
            {"1", "a"},
            {"2", "b"},
            {"3", "c"},
            {"4", "d"}
        };
        map<string, string> m2 = { 
            {"1", "a"},
            {"2", "b"},
            {"3", "c"}
        };
        auto result = IterCompare(
                        m1.begin(), m1.end(), m2.begin(), m2.end(),
                        StringPairCompare);
        EXPECT_EQ(1, result);
}
