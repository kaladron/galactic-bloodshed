
#include <GUnit.h>
#include <vector>
#include "storage/types.h"

GTEST("FieldPath Default") {
    SHOULD("Join should return empty string") {
        EXPECT_EQ("", FieldPath().join("/"));
    }
}

GTEST("FieldPath Constructor") {
    SHOULD("FieldPath constructor") {
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
}

GTEST("FieldPath Push") {
    SHOULD("Join should return string delimited") {
        FieldPath fp;
        EXPECT_EQ("", fp.join("/"));

        FieldPath fp2 = fp.push("a");
        EXPECT_EQ("a", fp2.join("/"));

        fp = fp2.push("b");
        EXPECT_EQ("a", fp2.join("."));
        EXPECT_EQ("a.b", fp.join("."));
    }
}
