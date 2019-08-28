
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

struct Address {
    string number;
    string street;
    string city;
    string zipcode;
    string region;
    string country;
    int address_type;
};

Value *addressToValue(const Address &address) {
    Value *out = new MapValue();
    out->Set("number", StringBoxer(address.number));
    out->Set("street", StringBoxer(address.street));
    out->Set("city", StringBoxer(address.city));
    out->Set("region", StringBoxer(address.region));
    out->Set("country", StringBoxer(address.country));
    out->Set("address_type", IntBoxer(address.address_type));
    return out;
}

bool valueToAddress(const Value *const input, Address &address) {
    return StringUnboxer(input->Get("number"), address.number) &&
           StringUnboxer(input->Get("street"), address.street) &&
           StringUnboxer(input->Get("city"), address.city) &&
           StringUnboxer(input->Get("region"), address.region) &&
           StringUnboxer(input->Get("country"), address.country) &&
           IntUnboxer(input->Get("address_type"), address.address_type);
}

struct Person {
    int id;
    string name;
    long dob;
    string gender;
    Address address;
};

Value *personToValue(const Person &person) {
    Value *out = new MapValue();
    out->Set("id", IntBoxer(person.id));
    out->Set("name", StringBoxer(person.name));
    out->Set("dob", LongBoxer(person.dob));
    out->Set("gender", StringBoxer(person.gender));
    out->Set("address", addressToValue(person.address));
    return out;
}

bool valueToPerson(const Value *const input, Person &person) {
    return IntUnboxer(input->Get("id"), person.id) &&
           StringUnboxer(input->Get("name"), person.name) &&
           LongUnboxer(input->Get("dob"), person.dob) &&
           StringUnboxer(input->Get("gender"), person.gender) &&
           valueToAddress(input->Get("address"), person.address);
}

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
    PersonSchema = new Schema("Person", PersonType, { FieldPath("id") });
    CompanySchema = new Schema("Company", CompanyType, { FieldPath("id") });
}

int main(int argc, char *argv[]) {
    initTypes();
    initSchemas();
    cout << "Num args: " << argc << endl;
    const char *filename = argc <= 1 ? "test.db" : argv[1];
    SQLStore store(filename);
    auto people = store.GetCollection(PersonSchema);
    auto companies = store.GetCollection(PersonSchema);

    Address a1;
    a1.address_type = 101;
    a1.number = "12345";
    a1.street = "Tennis Court";
    a1.city = "Wimbledon";
    a1.region = "Brexitford";
    a1.country = "Britain";
    Value *a1value = addressToValue(a1);
    ValueToJson(a1value, cout);
    people->Put(*a1value);

    Person p1;
    Value *p1value = personToValue(p1);
    people->Put(p1value);

    Person p2;
    MapValue p2value;
    auto key = StringBoxer("1");
    people->Get(*key, p2value);
    valueToPerson(&p2value, p2);
}

