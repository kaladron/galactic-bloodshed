
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef ENTITIES_H
#define ENTITIES_H

#include <cstdlib>
#include <memory>
#include <utility>
#include <tuple>
#include <list>
#include <string>
#include <type_traits>

using namespace std;

template<typename ... FieldTypes> using Key = tuple<FieldTypes...>;

#if 0

class Address {
    int number;
    string street;
    string city;
    string state;
    string country;
    string zipcode;
    datetime created_at;
    pair<int, float> somepair;
};

SQL needs entities and the fields and the types of these fields.
Note that are entites are objects that are more logical and have a lot more richer metadata in them.  These are *not* meant to be optimized or memory efficient.   For example we may convert a bunch of rows we have read from a DB with each column in the row actually being a pointer to *somewhere* in memory (effectively destroying cache locality), but this entity would have a *lot* of info about the entity that we can use to reflect about it (say types etc).

In the Address struct about, at run time we have no clue what fields are, what the field types are etc and what their values are (say if we want to iterate through them).  What is needed is something like:

Address a = ...;
Prototype p = a.getClass();

Field fields = p.getFields();
fields.count() == 6
fields[0].name == "number"
fields[0].type == Leaf "String"
fields[0].value(a) == a.number

*But* Address *wont* have a getClass method.   We need an AddressEntity that is of the form:

class AddressClass : EntityClass {
    FIELDS(
        LeafField<int>, "street_number",
        LeafField<string>, "street_name",
        LeafField<string>, "city",
        LeafField<string>, "country",
        LeafField<string>, "zipcode",
        LeafField<long>, "created_timestamp",
        LeafField<int>, "somepair_first",
        LeafField<float>, "somepair_second")

    FIELDS = [
        new Leaf<Int>("number"),
        new Leaf<string>("street"),
        ...
    ];
};

class AddressEntity : Entity {
    LeafField<Int> street_number;
    LeafField<string> street_name;
    LeafField<string> city;
    LeafField<string> state;
    LeafField<string> country;
    LeafField<string> zipcode;
    LeafField<long> created_timestamp;
    LeafField<int> somepair_first;
    LeafField<float> somepair_second;

    FIELDS = [
        new Leaf<Int>("number"),
        new Leaf<string>("street"),
        ...
    ];
};


Ok so above gives us an address prototype.  But it reallydoes need to "inherit" prototype as fields could just be a constructor param for Prototype.
Especially given fields[0].value(input) is not deifned as a Field has no way to return the value from a given input.

If a field had a way to get and set values in the underlying object that would be awesome as the ser/deser utils can do a lot awesome things.

But the point of an entity is that we are agnostic of in-mem representation.   For example, what would happen if fields above were "containers" for values of an instance instead being bound to a particular class.

This way we could have multiple representations that can extract from a single prototype (say via a macro or templates).

eg we can have the Address class above, and a different Address class below:

class Address2 {
public:
    int GetNumber() const;
    void SetNumber(int num) { ... }
    ...
private:
    int number;
    string street;
    string city;
    string state;
    string country;
    string zipcode;
    datetime created_at;
    pair<int, float> somepair;
};

Here all access is *only* via getters and setters.   We want both Address and Address2 to be populatable from an AddressEntity

How?

1. Do it manually

EntityToAddress(AddressEntity *ae, Address *a) {
    a.number = ae->street_number->Get();
    a.street = ae->street_name->Get();
    a.city = ae->city->Get();
    a.state = ae->state->Get();
    a.country = ae->country->Get();
    a.zipcode = ae->zipcode->Get();
    a.created_at = longtodate(ae->created_timestamp->Get());
    a.somepair = make_pair(ae->somepair_first->Get(), ae->somepair_second->Get());
}

// and Similarly the other way around

2. use macros to generate both 
#endif 

/**
 * Holds metadata about a field that is shared by *all* instances of a field with a given name in a given entity.
 * This ensures that multiple instances of an entity do not duplicate this but can benefit from a shared singleton
 * to provide this metadata (and shared behaviour on a field).
 */
class FieldClass {
public:
    FieldClass(const string &name) : fieldname(name) {}

private:
    string fieldname;
};

class EntityClass {
public:
    virtual string FQN() const = 0;
    virtual size_t FieldCount() const = 0;
    virtual FieldClass *GetField(size_t index) const = 0;
    virtual FieldClass *GetField(const string & name) const = 0;
private:
    EntityClass *base = nullptr;
};

class Field {
public:
    Field(const FieldClass *const fieldclass) : field_class(fieldclass) {}
    virtual ~Field() = 0;
    template <typename ValueType> const ValueType &Get();
    template <typename ValueType> const ValueType &Set();
private:
    const FieldClass *field_class;
};

template <typename E, enable_if_t<is_base_of<EntityClass, E>::value, void> = nullptr>
class Entity {
public:
    virtual ~Entity() = 0;
    virtual Field *GetField(const string &fieldname) const = 0;
    virtual const list<shared_ptr<Field>> &Fields() const = 0;
    virtual const list<shared_ptr<Field>> &KeyFields() const = 0;

    template<typename ...FieldTypes>
    const Key<FieldTypes...> &GetKey() const;

    template<typename ...FieldTypes>
    void SetKey(const Key<FieldTypes...> &value);

    template <class F> // enable_if_t<is_base_of<Field, F>::value, void>
    F *GetField(const string &fieldname) { return (F *)GetField(fieldname); }

private:
    E *entity_class;
};

#endif

