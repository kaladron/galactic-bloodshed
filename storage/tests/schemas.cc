
#include <gtest/gtest.h>
#include "storage/storage.h"

using std::cout;
using std::endl;
using std::make_shared;
using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static auto StringType = registry->GetType("string");
static auto Int64Type = registry->GetType("int64");
static auto Int32Type = registry->GetType("int32");
static auto Int16Type = registry->GetType("int16");
static auto BoolType = registry->GetType("bool");
static auto DateType = Int64Type;

// How do we want to test schemas?
// 1. Define a schema - say from a string or a file
// 2. Do write/read/update/delete/get tests

static shared_ptr<Store> setupTestDb(const char *dbpath = "/vagrant/storage.tests.db");
static void RegisterSchemas();

TEST(Schemas, SimpleSchemas) {
    auto store = setupTestDb();
    auto people = store->GetCollection(registry->GetSchema("Person").get());
    auto companies = store->GetCollection(registry->GetSchema("Company").get());

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
    auto AddressType = make_shared<Type>("Address", Type::ProductType({
                        pair("number", StringType),
                        pair("street", StringType),
                        pair("city", StringType),
                        pair("zipcode", StringType),
                        pair("region", StringType),
                        pair("country", StringType)
                    }));
    auto CompanyType = make_shared<Type>("Company", Type::ProductType({
                            pair("id", Int64Type),
                            pair("name", StringType),
                            pair("founded_on", DateType),
                            pair("hq", AddressType)
                        }));

    auto PersonType = make_shared<Type>("Person", Type::ProductType({
                        pair("id", Int64Type),
                        pair("name", StringType),
                        pair("dob", DateType),
                        pair("gender", StringType), // need enums
                        pair("address", AddressType)
                    }));
    registry->Add(AddressType);
    registry->Add(PersonType);
    registry->Add(CompanyType);

    auto company_schema = make_shared<Schema>("Company", CompanyType, vector<FieldPath>({ FieldPath("id") }));
    registry->Add(company_schema);

    auto person_schema = make_shared<Schema>("Person", PersonType, vector<FieldPath>({ FieldPath("id") }));
    registry->Add(person_schema);
};
