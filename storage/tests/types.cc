
#include <GUnit.h>
#include <vector>
#include "storage/types.h"

using namespace Storage;

auto StringType = DefaultTypes::StringType();
auto Int64Type = DefaultTypes::Int64Type();
auto Int32Type = DefaultTypes::Int32Type();

GTEST("TypeFunctions") {
    SHOULD("Work as TypeFunctions") {
        EXPECT_EQ("int", Type("int").FQN());
        EXPECT_EQ(true, Type("int").IsTypeFun());
        EXPECT_EQ(0, Type("int").ChildCount());
    }
}

GTEST("Record Types") {
    SHOULD("Record type Tests") {
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
}

GTEST("Union Types") {
    SHOULD("Union type Tests") {
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
}

