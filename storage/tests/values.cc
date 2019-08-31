
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
        EXPECT_EQ(val1, lv.Get(0));
        EXPECT_EQ(val2, lv.Get(1));
        EXPECT_EQ(val3, lv.Get(2));
        EXPECT_EQ(nullptr, lv.Get(3));
    }

    SHOULD("List HashCode should be sum of child value hash codes") {
        auto val1 = StringBoxer("hello world");
        auto val2 = Int8Boxer(42);
        auto val3 = BoolBoxer(true);
        vector<Value*> values = { val1, val2, val3 };
        ListValue lv(values);
        size_t h = 0;
        for (int i = 0;i < lv.ChildCount();i++) {
            h += lv.Get(i)->HashCode();
        }
        EXPECT_EQ(h, lv.HashCode());
    }

    SHOULD("List comparison compares values") {
        auto val1 = StringBoxer("hello world");
        auto val2 = Int8Boxer(42);
        auto val3 = BoolBoxer(true);
        vector<Value*> values = { val1, val2, val3 };
        ListValue lv1(values);
        ListValue lv2(values);
        EXPECT_EQ(0, lv1.Compare(&lv2));

        vector<Value*> values2 = { val1, val3, val2 };
        ListValue lv3(values2);
        EXPECT_EQ(val2->Compare(val3), lv2.Compare(&lv3));
    }
}

GTEST("Map Values") {
    SHOULD("Map value creation") {
        MapValue lv;
        EXPECT_EQ(0, lv.ChildCount());
        EXPECT_EQ(false, lv.HasChildren());
        EXPECT_EQ(false, lv.IsIndexed());
        EXPECT_EQ(true, lv.IsKeyed());
        EXPECT_EQ(0, lv.HashCode());
    }

    SHOULD("Map value with values") {
        auto val1 = StringBoxer("hello world");
        auto val2 = Int8Boxer(42);
        auto val3 = BoolBoxer(true);
        map<string, Value*> values = { 
            {"1", val1},
            {"2", val2},
            {"3", val3}
        };
        MapValue lv(values);
        EXPECT_EQ(3, lv.ChildCount());
        EXPECT_EQ(true, lv.HasChildren());
        EXPECT_EQ(false, lv.IsIndexed());
        EXPECT_EQ(true, lv.IsKeyed());
        EXPECT_EQ(val1, lv.Get("1"));
        EXPECT_EQ(val2, lv.Get("2"));
        EXPECT_EQ(val3, lv.Get("3"));
        EXPECT_EQ(nullptr, lv.Get("4"));
    }

    SHOULD("Map HashCode should be sum of child key and value hash codes") {
        auto val1 = StringBoxer("hello world");
        auto val2 = Int8Boxer(42);
        auto val3 = BoolBoxer(true);
        map<string, Value*> values = { 
            {"1", val1},
            {"2", val2},
            {"3", val3}
        };
        MapValue lv(values);
        hash<string> hasher;
        size_t h = 0;
        for (auto key : lv.Keys()) {
            h += hasher(key);
            h += lv.Get(key)->HashCode();
        }
        EXPECT_EQ(h, lv.HashCode());
    }

    SHOULD("Map comparison compares values") {
        auto val1 = StringBoxer("hello world");
        auto val2 = Int8Boxer(42);
        auto val3 = BoolBoxer(true);
        map<string, Value*> values = { 
            {"1", val1},
            {"2", val2},
            {"3", val3}
        };
        MapValue lv1(values);
        MapValue lv2(values);
        EXPECT_EQ(0, lv1.Compare(&lv2));

        map<string, Value*> values2 = { 
            {"1", val1},
            {"2", val3},    // -> order swapped here
            {"3", val2}
        };
        MapValue lv3(values2);
        EXPECT_EQ(val2->Compare(val3), lv2.Compare(&lv3));
    }
}

