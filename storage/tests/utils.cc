
#include <GUnit.h>
#include "storage/storage.h"

using namespace Storage;

GTEST("Comparer") {
    SHOULD("Should Compare Integers") {
        EXPECT_EQ(5, Comparer<int>()(10, 5));
    }

    SHOULD("Should Compare Strings") {
        string first = "hello";
        string second = "world";
        EXPECT_EQ(first.compare(second), Comparer<string>()(first, second));

        second = "hello";
        EXPECT_EQ(first.compare(second), Comparer<string>()(first, second));
    }
}

