
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

    SHOULD("Compare equal values") {
        auto val1 = StringBoxer("Hello");
        auto val2 = StringBoxer("Hello");
        EXPECT_EQ(true, val1->Equals(val2));
        EXPECT_EQ(0, val1->Compare(val2));
    }

    SHOULD("Compare unequal values") {
        auto val1 = StringBoxer("Hello");
        auto val2 = StringBoxer("World");
        EXPECT_EQ(false, val1->Equals(val2));
        EXPECT_EQ(-1, val1->Compare(val2));
    }

    SHOULD("Compare unequal literal types") {
        auto val1 = StringBoxer("Hello");
        auto val2 = Int8Boxer(42);
        cout << val1 << ", " << val2 << endl;
        EXPECT_EQ(false, val1->Equals(val2));
        EXPECT_EQ(val1->LitType() - val2->LitType(), val1->Compare(val2));
    }
}

GTEST("List Values") {
    SHOULD("List value creation") {
        ListValue lv;
        EXPECT_EQ(0, lv.ChildCount());
        EXPECT_EQ(false, lv.HasChildren());
        EXPECT_EQ(true, lv.IsIndexed());
        EXPECT_EQ(false, lv.IsKeyed());
        EXPECT_EQ(0, lv.HashCode());
    }

    SHOULD("List value with values") {
        auto val1 = StringBoxer("hello world");
        auto val2 = Int8Boxer(42);
        auto val3 = BoolBoxer(true);
        vector<Value*> values = { val1, val2, val3 };
        ListValue lv(values);
        EXPECT_EQ(3, lv.ChildCount());
        EXPECT_EQ(true, lv.HasChildren());
        EXPECT_EQ(true, lv.IsIndexed());
        EXPECT_EQ(false, lv.IsKeyed());
        EXPECT_EQ(val1->HashCode() + val2->HashCode() + val3->HashCode(),
                  lv.HashCode());
        EXPECT_EQ(val1, lv.Get(0));
        EXPECT_EQ(val2, lv.Get(1));
        EXPECT_EQ(val3, lv.Get(2));
        EXPECT_EQ(nullptr, lv.Get(3));
    }
}

