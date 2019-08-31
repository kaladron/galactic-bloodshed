
#include <GUnit.h>
#include "storage/storage.h"

using namespace Storage;

static auto StringType = DefaultTypes::StringType();
static auto LongType = DefaultTypes::LongType();
static auto IntType = DefaultTypes::IntType();
static auto DateType = LongType;

GTEST("Literals") {
    SHOULD("Verify literal types") {
        EXPECT_EQ(LiteralType::String, StringBoxer("test")->LitType());
        EXPECT_EQ(LiteralType::Int16, Int16Boxer(450)->LitType());
        EXPECT_EQ(LiteralType::Bool, BoolBoxer(0)->LitType());
    }

    SHOULD("Stringify normally") {
        EXPECT_EQ("hello world", StringBoxer("hello world")->AsString());
        EXPECT_EQ("42", Int8Boxer(42)->AsString());
        EXPECT_EQ("true", BoolBoxer(true)->AsString());
        EXPECT_EQ("false", BoolBoxer(0)->AsString());
    }

    SHOULD("Unbox should match box") {
        auto input = "Hello World";
        auto boxed = StringBoxer(input);
        string output;
        EXPECT_EQ(LiteralType::String, boxed->LitType());
        EXPECT_EQ(true, StringUnboxer(boxed, output));
        EXPECT_EQ(input, output);
    }

    SHOULD("Unbox should fail on wrong box") {
        auto input = "Hello World";
        auto boxed = StringBoxer(input);
        int8_t output;
        EXPECT_EQ(false, Int8Unboxer(boxed, output));
    }

    SHOULD("Call standard hash method") {
        auto value = StringBoxer("hello world");
        EXPECT_EQ(hash<string>()("hello world"), value->HashCode());

        auto value2 = Int8Boxer(42);
        EXPECT_EQ(hash<int>()(42), value2->HashCode());

        auto value3 = FloatBoxer(42.5);
        EXPECT_EQ(hash<float>()(42.5), value3->HashCode());
    }

    SHOULD("Compare equal types") {
        auto val1 = StringBoxer("Hello");
        auto val2 = StringBoxer("Hello");
        EXPECT_EQ(true, val1->Equals(val2));
        EXPECT_EQ(0, val1->Compare(val2));
    }

    SHOULD("Compare unequal types") {
        auto val1 = StringBoxer("Hello");
        auto val2 = StringBoxer("World");
        EXPECT_EQ(false, val1->Equals(val2));
        EXPECT_EQ(-1, val1->Compare(val2));
    }
}

