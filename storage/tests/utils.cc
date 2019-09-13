
#include <GUnit.h>
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

GTEST("Comparer") {
    SHOULD("Compare Integers") {
        EXPECT_EQ(5, Comparer<int>()(10, 5));
    }

    SHOULD("Compare Strings") {
        string first = "hello";
        string second = "world";
        EXPECT_EQ(first.compare(second), Comparer<string>()(first, second));

        second = "hello";
        EXPECT_EQ(first.compare(second), Comparer<string>()(first, second));
    }
}


GTEST("Joiner") {
    SHOULD("Join Strings") {
        EXPECT_EQ("a/b/c/d", joinStrings({"a", "b", "c", "d"}, "/"));
        EXPECT_EQ("a##b##c##d", joinStrings({"a", "b", "c", "d"}, "##"));
    }

    SHOULD("Not Ignore empty Strings") {
        EXPECT_EQ("a//c/", joinStrings({"a", "", "c", ""}, "/"));
    }
}

GTEST("IterCompare") {
    SHOULD("Compare Equal Vectors") {
        vector<int> v1 = {1,2,3};
        vector<int> v2 = {1,2,3};
        auto result = IterCompare(
                        v1.begin(), v1.end(), v2.begin(), v2.end(),
                        [](int a, int b) {
                            return a - b;
                        });
        EXPECT_EQ(0, result);
    }

    SHOULD("Compare Vectors - v1 < v2 should be -1") {
        vector<int> v1 = {1,2,3};
        vector<int> v2 = {1,2,3,4};
        auto result = IterCompare(
                        v1.begin(), v1.end(), v2.begin(), v2.end(),
                        [](int a, int b) {
                            return a - b;
                        });
        EXPECT_EQ(-1, result);
    }

    SHOULD("Compare Vectors - v1 > v2 should be >1") {
        vector<int> v1 = {1,2,10};
        vector<int> v2 = {1,2,3,4};
        auto result = IterCompare(
                        v1.begin(), v1.end(), v2.begin(), v2.end(),
                        [](int a, int b) {
                            return a - b;
                        });
        EXPECT_EQ(true, result > 0);
    }

    SHOULD("Compare Equal Maps") {
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

    SHOULD("Compare Maps - m1 < m2 should be -1") {
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

    SHOULD("Compare Vectors - v1 > v2 should be >1") {
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
}
