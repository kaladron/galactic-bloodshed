
#include <gtest/gtest.h>
#include <vector>
#include "storage/types.h"

using namespace Storage;

TEST(FieldPathDefault, JoinShouldReturnEmptyString) {
        EXPECT_EQ("", FieldPath().join("/"));
}

TEST(FieldPathConstructor, FieldPathConstructor) {
        FieldPath fp("a/b/c/d/e", "/");
        EXPECT_EQ("a--b--c--d--e", fp.join("--"));
        EXPECT_EQ("a+b+c+d+e", fp.join("+"));

        std::vector<string> inv = { "a", "b", "c" };
        FieldPath fp2(inv);
        EXPECT_EQ("a-b-c", fp2.join("-"));

        FieldPath fp3(fp2);
        fp3.push_back("d");
        EXPECT_EQ("a-b-c", fp2.join("-"));
        EXPECT_EQ("a-b-c-d", fp3.join("-"));
}

TEST(FieldPathPush, JoinShouldReturnStringDelimited) {
        FieldPath fp;
        EXPECT_EQ("", fp.join("/"));

        FieldPath fp2 = fp.push("a");
        EXPECT_EQ("a", fp2.join("/"));

        fp = fp2.push("b");
        EXPECT_EQ("a", fp2.join("."));
        EXPECT_EQ("a.b", fp.join("."));
}

