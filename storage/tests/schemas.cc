
#include <gtest/gtest.h>
#include "storage/storage.h"

using std::cout;
using std::endl;
using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static const Type *StringType = registry->GetType("string");
static const Type *Int64Type = registry->GetType("int64");
static const Type *Int32Type = registry->GetType("int32");
static const Type *Int16Type = registry->GetType("int16");
static const Type *BoolType = registry->GetType("bool");
static const Type *DateType = Int64Type;

// How do we want to test schemas?
// 1. Define a schema - say from a string or a file
// 2. Do write/read/update/delete/get tests

static shared_ptr<Store> setupTestDb(const char *dbpath = "/vagrant/storage.tests.db");
static void RegisterSchemas();

TEST(Schemas, SimpleSchemas) {
    auto store = setupTestDb();
    auto people = store->GetCollection(registry->GetSchema("Person"));
    auto companies = store->GetCollection(registry->GetSchema("Company"));

    StrongValue address1(new MapValue({
        { "address_type", UInt8Boxer(101) },    // invalid field - should not be persisted
        { "number", StringBoxer("12345") },
        { "street", StringBoxer("Tennis Court") },
        { "city", StringBoxer("Wimbledon") },
        { "region", StringBoxer("London") },
        { "country", StringBoxer("UK") },
    }));

    StrongValue person1(new MapValue({
        { "id", Int64Boxer(666) },
        { "name", StringBoxer("Hell Boy") },
        { "gender", StringBoxer("N") },
        { "address", address1 }
    }));
    people->Put(person1);

    // Now read it back
    StrongValue key = Int64Boxer(666);
    StrongValue p3value = people->Get(key);

    EXPECT_EQ(666, Int64Unboxer(p3value->Get("id")));
    EXPECT_EQ("Hell Boy", StringUnboxer(p3value->Get("name")));
    EXPECT_EQ("N", StringUnboxer(p3value->Get("gender")));
    auto address2 = p3value->Get("address");
    auto address_type = address2->Get("address_type");
    EXPECT_EQ(nullptr, address_type);       // address type should not be persisted
    EXPECT_EQ("12345", StringUnboxer(address2->Get("number")));
    EXPECT_EQ("Tennis Court", StringUnboxer(address2->Get("street")));
    EXPECT_EQ("Wimbledon", StringUnboxer(address2->Get("city")));
    EXPECT_EQ("London", StringUnboxer(address2->Get("region")));
    EXPECT_EQ("UK", StringUnboxer(address2->Get("country")));
}

//////////////////  Helper methods

shared_ptr<Store> setupTestDb(const char *dbpath) {
    remove(dbpath);
    RegisterSchemas();
    return std::make_shared<SQLStore>(dbpath);
}

void RegisterSchemas() {
    auto AddressType = new Type("Address", {
                        NameTypePair("number", StringType),
                        NameTypePair("street", StringType),
                        NameTypePair("city", StringType),
                        NameTypePair("zipcode", StringType),
                        NameTypePair("region", StringType),
                        NameTypePair("country", StringType)
                    });
    auto CompanyType = new Type("Company", {
                            NameTypePair("id", Int64Type),
                            NameTypePair("name", StringType),
                            NameTypePair("founded_on", DateType),
                            NameTypePair("hq", AddressType)
                        });

    auto PersonType = new Type("Person", {
                        NameTypePair("id", Int64Type),
                        NameTypePair("name", StringType),
                        NameTypePair("dob", DateType),
                        NameTypePair("gender", StringType), // need enums
                        NameTypePair("address", AddressType)
                    });
    registry->Add(AddressType);
    registry->Add(PersonType);
    registry->Add(CompanyType);

    registry->Add(new Schema("Company", CompanyType, { FieldPath("id") }));
    registry->Add(new Schema("Person", PersonType, { FieldPath("id") }));
};
