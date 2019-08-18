
#include "storage/storage.h"
#include <iostream>

using namespace Storage;

const Type *StringType = nullptr;
const Type *LongType = nullptr;
const Type *IntType = nullptr;
const Type *DateType = nullptr;
const Type *AddressType = nullptr;
const Type *PersonType = nullptr;
const Type *CompanyType = nullptr;
const Schema *PersonSchema = nullptr;
const Schema *CompanySchema = nullptr;

void initTypes() {
    StringType = DefaultTypes::StringType();
    LongType = DefaultTypes::LongType();
    IntType = DefaultTypes::IntType();
    DateType = LongType;

    AddressType = new Type("Address", {
                            NameTypePair("number", StringType),
                            NameTypePair("street", StringType),
                            NameTypePair("city", StringType),
                            NameTypePair("zipcode", StringType),
                            NameTypePair("region", StringType),
                            NameTypePair("country", StringType)
                        });

    CompanyType = new Type("Company", {
                            NameTypePair("id", IntType),
                            NameTypePair("name", StringType),
                            NameTypePair("founded_on", DateType),
                            NameTypePair("hq", AddressType),
                        });

    PersonType = new Type("Person", {
                            NameTypePair("id", IntType),
                            NameTypePair("name", StringType),
                            NameTypePair("dob", DateType),
                            NameTypePair("gender", StringType), // need enums
                        });
}

void initSchemas() {
    PersonSchema = new Schema("Person", PersonType, { "id" });
    CompanySchema = new Schema("Company", CompanyType, { "id" });
}

int main(int argc, char *argv[]) {
    initTypes();
    initSchemas();
    cout << "Num args: " << argc << endl;
    const char *filename = argc <= 1 ? "test.db" : argv[1];
    SQLStore store(filename);
    auto people = store.GetCollection(PersonSchema);
    auto companies = store.GetCollection(PersonSchema);
}

