
#include <gtest/gtest.h>
#include "storage/storage.h"

template <typename T>
using hash = std::hash<T>;

using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static const Type *StringType = registry->GetType("string");
static const Type *Int32Type = registry->GetType("int32");
static const Type *Int16Type = registry->GetType("int16");
static const Type *BoolType = registry->GetType("bool");

TEST(Literals, VerifyLiteralTypes) {
    EXPECT_EQ(LiteralType::String, StringBoxer("test")->LitType());
    EXPECT_EQ(LiteralType::Int16, Int16Boxer(450)->LitType());
    EXPECT_EQ(LiteralType::Bool, BoolBoxer(0)->LitType());
}

TEST(Literals, StringifyNormally) {
    EXPECT_EQ("hello world", StringBoxer("hello world")->AsString());
    EXPECT_EQ("42", Int8Boxer(42)->AsString());
    EXPECT_EQ("true", BoolBoxer(true)->AsString());
    EXPECT_EQ("false", BoolBoxer(0)->AsString());
}

TEST(Literals, UnboxShouldMatchBox) {
    auto input = "Hello World";
    auto boxed = StringBoxer(input);
    string output;
    EXPECT_EQ(LiteralType::String, boxed->LitType());
    EXPECT_EQ(true, StringUnboxer(boxed, output));
    EXPECT_EQ(input, output);
}

TEST(Literals, UnboxShouldFailOnWrongBox) {
    auto input = "Hello World";
    auto boxed = StringBoxer(input);
    int8_t output;
    EXPECT_EQ(false, Int8Unboxer(boxed, output));
}

TEST(Literals, CallStandardHashMethod) {
    StrongValue value = StringBoxer("hello world");
    EXPECT_EQ(hash<string>()("hello world"), value->HashCode());

    StrongValue value2 = Int8Boxer(42);
    EXPECT_EQ(hash<int>()(42), value2->HashCode());

    StrongValue value3 = FloatBoxer(42.5);
    EXPECT_EQ(hash<float>()(42.5), value3->HashCode());
}

TEST(Literals, CompareEqualValues) {
    StrongValue val1 = StringBoxer("Hello");
    StrongValue val2 = StringBoxer("Hello");
    EXPECT_EQ(true, val1->Equals(val2));
    EXPECT_EQ(0, val1->Compare(val2));
}

TEST(Literals, CompareUnequalValues) {
    StrongValue val1 = StringBoxer("Hello");
    StrongValue val2 = StringBoxer("World");
    EXPECT_EQ(false, val1->Equals(val2));
    EXPECT_EQ(-1, val1->Compare(val2));
}

TEST(Literals, CompareUnequalLiteralTypes) {
    StrongLiteral val1 = StringBoxer("Hello");
    StrongLiteral val2 = Int8Boxer(42);
    EXPECT_EQ(false, val1->Equals(val2));
    EXPECT_EQ(val1->LitType() - val2->LitType(), val1->Compare(val2.get()));
}

TEST(ListValues, ListValueCreation) {
    ListValue lv;
    EXPECT_EQ(0, lv.ChildCount());
    EXPECT_EQ(false, lv.HasChildren());
    EXPECT_EQ(true, lv.IsIndexed());
    EXPECT_EQ(false, lv.IsKeyed());
    EXPECT_EQ(0, lv.HashCode());
}

TEST(ListValues, ListValueWithValues) {
    StrongValue val1 = StringBoxer("hello world");
    StrongValue val2 = Int8Boxer(42);
    StrongValue val3 = BoolBoxer(true);
    ValueVector values = { val1, val2, val3 };
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

TEST(ListValues, ListHashCodeShouldBeSumOfChildValueHashCodes) {
    StrongValue val1 = StringBoxer("hello world");
    StrongValue val2 = Int8Boxer(42);
    StrongValue val3 = BoolBoxer(true);
    ValueVector values = { val1, val2, val3 };
    ListValue lv(values);
    size_t h = 0;
    for (int i = 0;i < lv.ChildCount();i++) {
        h += lv.Get(i)->HashCode();
    }
    EXPECT_EQ(h, lv.HashCode());
}

TEST(ListValues, ListComparisonComparesValues) {
    StrongValue val1 = StringBoxer("hello world");
    StrongValue val2 = Int8Boxer(42);
    StrongValue val3 = BoolBoxer(true);
    ValueVector values = { val1, val2, val3 };
    ListValue lv1(values);
    ListValue lv2(values);
    EXPECT_EQ(0, lv1.Compare(&lv2));

    ValueVector values2 = { val1, val3, val2 };
    ListValue lv3(values2);
    EXPECT_EQ(val2->Compare(val3), lv2.Compare(&lv3));
}

TEST(MapValues, MapValueCreation) {
    MapValue lv;
    EXPECT_EQ(0, lv.ChildCount());
    EXPECT_EQ(false, lv.HasChildren());
    EXPECT_EQ(false, lv.IsIndexed());
    EXPECT_EQ(true, lv.IsKeyed());
    EXPECT_EQ(0, lv.HashCode());
}

TEST(MapValues, MapValueWithValues) {
    StrongValue val1 = StringBoxer("hello world");
    StrongValue val2 = Int8Boxer(42);
    StrongValue val3 = BoolBoxer(true);
    ValueMap values = { 
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

TEST(MapValues, MapHashCodeShouldBeSumOfChildKeyAndValueHashCodes) {
    StrongValue val1 = StringBoxer("hello world");
    StrongValue val2 = Int8Boxer(42);
    StrongValue val3 = BoolBoxer(true);
    ValueMap values = { 
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

TEST(MapValues, MapComparisonComparesValues) {
    StrongValue val1 = StringBoxer("hello world");
    StrongValue val2 = Int8Boxer(42);
    StrongValue val3 = BoolBoxer(true);
    ValueMap values = { 
        {"1", val1},
        {"22", val2},
        {"3", val3}
    };
    MapValue lv1(values);
    MapValue lv2(values);
    EXPECT_EQ(0, lv1.Compare(&lv2));

    ValueMap values2 = { 
        {"1", val1},
        {"22", val3},    // -> order swapped here
        {"3", val2}
    };
    MapValue lv3(values2);
    EXPECT_EQ(val2->Compare(val3), lv2.Compare(&lv3));
}
