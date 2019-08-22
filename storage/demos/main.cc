
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

#if 0
struct Address : public Value {
    string number;
    string street;
    string city;
    int age;
    string zipcode;
    string region;
    string country;

    virtual size_t HashCode() const { return 0; }
    virtual int Compare(const Value &another) const {
        return 0;
    }

    /**
     * Values can be containers.
     */
    virtual size_t ChildCount() const { return 6; } // same as # fields
    virtual bool IsKeyed() const { return true; }
    virtual bool IsIndexed() const { return true; }
    virtual Value *ChildAt(size_t index) const { 
        switch (index) {
            case 0: return MakeValue(number);
            case 1: return MakeValue(street);
            case 2: return MakeValue(city);
            case 3: return MakeValue(age);
        }
        return nullptr;
    }
    virtual Value *ChildFor(const std::string &key) const {
        if (key == "number") return MakeValue(number);
        if (key == "street") return MakeValue(street);
        if (key == "city") return MakeValue(city);
        return nullptr;
    }
    // Setters
    // virtual Value *SetChildAt(size_t index, Value *newvalue) { }
    // virtual Value *SetChildFor(const std::string &key, Value *newvalue) { }

};

struct Person : public Value {
    int id;
    string name;
    date dob;
    string gender;
    Address address;

    size_t ChildCount() const { return 4; } // same as # fields
    bool IsKeyed() const { return true; }
    bool IsIndexed() const { return true; }
    virtual Value *ChildAt(size_t index) const { 
        switch (index) {
            case 0: return MakeValue(id);
            case 1: return MakeValue(name);
            case 2: return MakeValue(dob);
            case 3: return MakeValue(gender);
            case 4: return &address;
        }
        return nullptr;
    }
    virtual Value *ChildFor(const std::string &key) const {
        if (key == "id") return MakeValue(id);
        if (key == "number") return MakeValue(number);
        if (key == "dob") return MakeValue(dob);
        if (key == "gender") return MakeValue(gender);
        if (key == "address") return &address;
        return nullptr;
    }
};
#endif

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

#if 0
    Address a1;
    people->Put(a1);    // would be great if this failed compilation as 
                        // people table works with Person schema 
                        // and address is not a valid DAO
                        // Will have to settle for schema validation

    Person p1;
    people->Put(p1);    // would be great if this failed compilation as 
    Person p2;
    people->Get(MakeValue(1), p2);

    can something like this help:
    // We want to be able to work with what ever tables "return"
    // currently these are entities, but entities used to be
    // maps.  Should they be?  Why not just Values?
    // Can Entities just be interfaces for getting/setting keys?
    // eg:

    // with Person being:
    // class Person : Value {
    //      fields generated however (say via macros or schema spec etc).
    // }
    Person *p1 = people->Get(make_key(3));

    // The reason we needed "entity" above was because we needed a 
    // boundary interface object.  But if we could pass an entity
    // interface (or a value interface) to be populated, then the
    // table wouldnt be responsible for allocating an object, so 
    // our call could be:
    Person p1;
    people->Get(make_key(3), p1);   // returns true/false for exists or not

    And p1 as a value can just be mutable too with setters!

    So do we need Entities?  Yes.  Values on their own have no "keys" and other store related facilities
    Entities are this interface between Values in the "real" world and what a table needs.

    Entity *p1 = people->Get(3);
    //Entity *c1 = companies->Get(3);
    // Should be null

    But should Entity be an interface or a wrapper class?

        Goal is we want the DB side to have an interface into an object that is exposed to app side 

    eg if we had a Person class like:

    class Person : public <SomeInterfaceRequiredByStore> {
        // have all data here as normal
        int id;
        string firstname;
        string lastname;
        Address address;    // Address also implements 
                            // "SomeInterfaceRequiredByStore"
        Ref<Employer> employer;

        // Implement the Value interface methods "manually"
        ChildCount() { return 4; /* or how many ever fields we have */ }
        ChildAt(index) {
            switch (index) {
                0 => shared_ptr(new LeafValue(id))
                1 => shared_ptr(new LeafValue(firstname))
                2 => shared_ptr(new LeafValue(lastname))
                3 => shared_ptr(&address);
            }
        }
    }

    We could then have:

        Person p1(firstname = "a", lastname = "b", id = 3, address = Address( ... ));
        auto people = store.GetCollection(PeopleSchema);
        people.put(&p1);

        or people.get(&p1);     // people table already has the schema and hence the type


#endif
}

