
#include "storage/storage.h"
#include <iostream>

using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static auto StringType = registry->GetType("string");
static auto Int64Type = registry->GetType("int64");
static auto DateType = Int64Type;

static StrongType AddressType = nullptr;
static StrongType PersonType = nullptr;
static StrongType CompanyType = nullptr;
static shared_ptr<Schema> PersonSchema = nullptr;
static shared_ptr<Schema> CompanySchema = nullptr;

using std::cout;
using std::endl;
using std::make_shared;

struct Address {
    string number;
    string street;
    string city;
    string zipcode;
    string region;
    string country;
    uint8_t address_type;
};

struct Person {
    uint32_t id;
    string name;
    long dob;
    string gender;
    Address address;
};

void initTypes();
void initSchemas();
StrongValue addressToValue(const Address &address);
bool valueToAddress(StrongValue input, Address &address);
StrongValue personToValue(const Person &person);
bool valueToPerson(StrongValue input, Person &person);

StrongValue addressToValue(const Address &address) {
    auto out = make_shared<MapValue>();
    out->Set("number", StringBoxer(address.number));
    out->Set("street", StringBoxer(address.street));
    out->Set("city", StringBoxer(address.city));
    out->Set("region", StringBoxer(address.region));
    out->Set("country", StringBoxer(address.country));
    out->Set("address_type", UInt8Boxer(address.address_type));
    return out;
}

bool valueToAddress(StrongValue input, Address &address) {
    return StringUnboxer(input->Get("number"), address.number) &&
           StringUnboxer(input->Get("street"), address.street) &&
           StringUnboxer(input->Get("city"), address.city) &&
           StringUnboxer(input->Get("region"), address.region) &&
           StringUnboxer(input->Get("country"), address.country) &&
           UInt8Unboxer(input->Get("address_type"), address.address_type);
}

StrongValue personToValue(const Person &person) {
    auto out = make_shared<MapValue>();
    out->Set("id", UInt32Boxer(person.id));
    out->Set("name", StringBoxer(person.name));
    out->Set("dob", Int64Boxer(person.dob));
    out->Set("gender", StringBoxer(person.gender));
    out->Set("address", addressToValue(person.address));
    return out;
}

bool valueToPerson(StrongValue input, Person &person) {
    return UInt32Unboxer(input->Get("id"), person.id) &&
           StringUnboxer(input->Get("name"), person.name) &&
           Int64Unboxer(input->Get("dob"), person.dob) &&
           StringUnboxer(input->Get("gender"), person.gender) &&
           valueToAddress(input->Get("address"), person.address);
}

void initTypes() {
    AddressType = make_shared<Type>("Address", Type::ProductType({
                            pair("number", StringType),
                            pair("street", StringType),
                            pair("city", StringType),
                            pair("zipcode", StringType),
                            pair("region", StringType),
                            pair("country", StringType)
                        }));

    CompanyType = make_shared<Type>("Company", Type::ProductType({
                            pair("id", Int64Type),
                            pair("name", StringType),
                            pair("founded_on", DateType),
                            pair("hq", AddressType),
                        }));

    PersonType = make_shared<Type>("Person", Type::ProductType({
                            pair("id", Int64Type),
                            pair("name", StringType),
                            pair("dob", DateType),
                            pair("gender", StringType), // need enums
                            pair("address", AddressType), // need enums
                        }));
}

void initSchemas() {
    CompanySchema = make_shared<Schema>("Company", CompanyType, vector<FieldPath>({ FieldPath("id") }));
    PersonSchema = make_shared<Schema>("Person", PersonType, vector<FieldPath>({ FieldPath("id") }));
}

int main(int argc, char *argv[]) {
    initTypes();
    initSchemas();
    cout << "Num args: " << argc << endl;
    const char *filename = argc <= 1 ? "/vagrant/test.db" : argv[1];
    SQLStore store(filename);
    auto people = store.GetCollection(PersonSchema.get()).get();

    Address a1;
    a1.address_type = 101;
    a1.number = "12345";
    a1.street = "Tennis Court";
    a1.city = "Wimbledon";
    a1.region = "Brexitford";
    a1.country = "Britain";
    StrongValue a1value(addressToValue(a1));
    ValueToJson(a1value.get(), std::cout); std::cout << std::endl;
    people->Put(a1value);      // false ret val is an error - TODO: turn into exceptions

    Person p1;
    StrongValue p1value(personToValue(p1));
    people->Put(p1value);

    p1.id = 666;
    p1.name = "Hell Boy";
    p1.name = "11111";
    p1.gender = "N";
    p1.address.number = "1";
    p1.address.street = "Hell Lane";
    p1.address.region = "Sulphur Zone";
    p1.address.country = "Outworld";
    StrongValue p2value(personToValue(p1));
    people->Put(p2value);

    Person p2;
    StrongValue key(Int32Boxer(666));
    StrongValue p3value(people->Get(key));
    valueToPerson(p3value, p2);

    // test cases:
}

