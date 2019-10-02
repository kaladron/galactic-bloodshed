
#include <gtest/gtest.h>
#include "storage/storage.h"

using std::make_shared;
using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static auto StringType = registry->GetType("string");
static auto Int64Type = registry->GetType("int64");
static auto Int32Type = registry->GetType("int32");
static auto Int16Type = registry->GetType("int16");
static auto BoolType = registry->GetType("bool");

TEST(TypeFunctions, WorkAsTypeFunctions) {
        EXPECT_EQ("int", Type("int").FQN());
        EXPECT_EQ(true, Type("int").IsTypeFun());
        EXPECT_EQ(0, Type("int").ChildCount());
}

TEST(RecordTypes, ProductTypeTests) {
        auto AddressType(make_shared<Type>("Address", Type::ProductType({
                                NameTypePair("number", StringType),
                                NameTypePair("street", StringType),
                                NameTypePair("city", StringType),
                                NameTypePair("zipcode", StringType),
                                NameTypePair("region", StringType),
                                NameTypePair("country", StringType)
                            })));
        EXPECT_EQ("Address", AddressType->FQN());
        EXPECT_EQ(true, AddressType->IsProductType());
        auto x = AddressType->GetChild(0);
        EXPECT_EQ("number", x.first);
        EXPECT_EQ(StringType, x.second.lock());
}

TEST(UnionTypes, SumTypeTests) {
        auto MyUnion(make_shared<Type>("MyUnion", Type::SumType({
            NameTypePair("a", StringType),
            NameTypePair("b", Int32Type),
            NameTypePair("c", Int64Type),
        })));
        EXPECT_EQ("MyUnion", MyUnion->FQN());
        EXPECT_EQ(true, MyUnion->IsSumType());
        auto x = MyUnion->GetChild(0);
        EXPECT_EQ("a", x.first);
        EXPECT_EQ(StringType, x.second.lock());

        x = MyUnion->GetChild(1);
        EXPECT_EQ("b", x.first);
        EXPECT_EQ(Int32Type, x.second.lock());
}

