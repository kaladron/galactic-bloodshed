
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
    EXPECT_EQ("int16", Int16Type->FQN());
    EXPECT_EQ("int", Type("int").FQN());
    EXPECT_EQ(true, Type("int").IsTypeFun());
    EXPECT_EQ(0, Type("int").ChildCount());
}

TEST(RecordTypes, ProductTypeTests) {
    auto AddressType(make_shared<Type>("Address", Type::ProductType({
        pair<string, StrongType>("number", StringType),
        pair<string, StrongType>("street", StringType),
        pair<string, StrongType>("city", StringType),
        pair<string, StrongType>("zipcode", StringType),
        pair<string, StrongType>("region", StringType),
        pair<string, StrongType>("country", StringType)
    })));
    EXPECT_EQ("Address", AddressType->FQN());
    EXPECT_EQ(true, AddressType->IsProductType());
    auto x = AddressType->GetChild(0);
    EXPECT_EQ("number", x.first);
    EXPECT_EQ(StringType, x.second);
}

TEST(UnionTypes, SumTypeTests) {
        auto MyUnion(make_shared<Type>("MyUnion", Type::SumType({
            pair("a", StringType),
            pair("b", Int32Type),
            pair("c", Int64Type),
        })));
        EXPECT_EQ("MyUnion", MyUnion->FQN());
        EXPECT_EQ(true, MyUnion->IsSumType());
        auto x = MyUnion->GetChild(0);
        EXPECT_EQ("a", x.first);
        EXPECT_EQ(StringType, x.second);

        x = MyUnion->GetChild(1);
        EXPECT_EQ("b", x.first);
        EXPECT_EQ(Int32Type, x.second);
}

