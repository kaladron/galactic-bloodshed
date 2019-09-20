
#include <gtest/gtest.h>
#include "storage/storage.h"

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

shared_ptr<Store> setupTestDb(const char *dbpath = "/tmp/storage.tests.db");

shared_ptr<Store> setupTestDb(const char *dbpath) {
    remove(dbpath);
    return std::make_shared<SQLStore>(dbpath);
}

struct TestSchemas {
    static const Type *ADDRESS_TYPE;
    static const Type *PERSON_TYPE;
    static const Type *COMPANY_TYPE;
    static const Schema *PERSON_SCHEMA;
    static const Schema *COMPANY_SCHEMA;

    static auto AddressType() {
        /*
        auto schema = R"SCHEMA(
            record Address {
                number : string,
                street : string,
                city : string,
                zipcode : string,
                region : string,
                country : string,
            }
        )SCHEMA";
        // this should give us a type registry of things we have
        parse_schemas(registry, schemas);
        register_schemas
        */
        if (!ADDRESS_TYPE) {
            ADDRESS_TYPE = new Type("Address", {
                                    NameTypePair("number", StringType),
                                    NameTypePair("street", StringType),
                                    NameTypePair("city", StringType),
                                    NameTypePair("zipcode", StringType),
                                    NameTypePair("region", StringType),
                                    NameTypePair("country", StringType)
                                });
        }
        return ADDRESS_TYPE;
    }

    static auto CompanyType() {
        if (!COMPANY_TYPE) {
            COMPANY_TYPE = new Type("Company", {
                                NameTypePair("id", Int64Type),
                                NameTypePair("name", StringType),
                                NameTypePair("founded_on", DateType),
                                NameTypePair("hq", AddressType()),
                            });
        }
        return COMPANY_TYPE;
    }

    static auto PersonType() {
        if (!PERSON_TYPE) {
            PERSON_TYPE = new Type("Person", {
                                NameTypePair("id", Int64Type),
                                NameTypePair("name", StringType),
                                NameTypePair("dob", DateType),
                                NameTypePair("gender", StringType), // need enums
                                NameTypePair("address", AddressType()), // need enums
                            });
        }
        return PERSON_TYPE;
    }

    static auto CompanySchema() {
        if (!COMPANY_SCHEMA) {
            COMPANY_SCHEMA = new Schema("Company", CompanyType(), { FieldPath("id") });
        }
        return COMPANY_SCHEMA;
    }

    static auto PersonSchema() {
        if (!PERSON_SCHEMA) {
            PERSON_SCHEMA = new Schema("Person", PersonType(), { FieldPath("id") });
        }
        return PERSON_SCHEMA;
    }
};

TEST(Schemas, SimpleSchemas) {
}

