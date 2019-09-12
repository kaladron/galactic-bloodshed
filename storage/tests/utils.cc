
#include <GUnit.h>
#include "storage/storage.h"

using namespace Storage;

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
}
