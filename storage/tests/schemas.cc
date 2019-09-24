
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
        { "address_type", UInt8Boxer(101) },
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

    int64_t val;
    auto idval = p3value->Get("id");
    cout << p3value << ", " << idval << endl;
    ValueToJson(p3value.get(), cout); cout << endl;
    EXPECT_NE(nullptr, idval);
    EXPECT_EQ(true, Int64Unboxer(idval, val));
    EXPECT_EQ(666, val);
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
