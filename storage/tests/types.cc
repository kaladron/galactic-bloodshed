
#include <gtest/gtest.h>
#include "storage/storage.h"

using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static const Type *StringType = registry->GetType("string");
static const Type *Int64Type = registry->GetType("int64");
static const Type *Int32Type = registry->GetType("int32");
static const Type *Int16Type = registry->GetType("int16");
static const Type *BoolType = registry->GetType("bool");

TEST(TypeFunctions, WorkAsTypeFunctions) {
        EXPECT_EQ("int", Type("int").FQN());
        EXPECT_EQ(true, Type("int").IsTypeFun());
        EXPECT_EQ(0, Type("int").ChildCount());
}

TEST(RecordTypes, RecordTypeTests) {
        unique_ptr<Type> AddressType(new Type("Address", {
                                NameTypePair("number", StringType),
                                NameTypePair("street", StringType),
                                NameTypePair("city", StringType),
                                NameTypePair("zipcode", StringType),
                                NameTypePair("region", StringType),
                                NameTypePair("country", StringType)
                            }));
        EXPECT_EQ("Address", AddressType->FQN());
        EXPECT_EQ(true, AddressType->IsRecord());
        auto x = AddressType->GetChild(0);
        EXPECT_EQ("number", x.first);
        EXPECT_EQ(StringType, x.second);
}

TEST(UnionTypes, UnionTypeTests) {
        unique_ptr<Type> MyUnion(new Type("MyUnion", {
                                NameTypePair("a", StringType),
                                NameTypePair("b", Int32Type),
                                NameTypePair("c", Int64Type),
                            },false));
        EXPECT_EQ("MyUnion", MyUnion->FQN());
        EXPECT_EQ(true, MyUnion->IsUnion());
        auto x = MyUnion->GetChild(0);
        EXPECT_EQ("a", x.first);
        EXPECT_EQ(StringType, x.second);

        x = MyUnion->GetChild(1);
        EXPECT_EQ("b", x.first);
        EXPECT_EQ(Int32Type, x.second);
}

