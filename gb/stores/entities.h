
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
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <type_traits>

using namespace std;

template<typename ... FieldTypes> using Key = tuple<FieldTypes...>;

/**
 * Holds metadata about a field that is shared by *all* instances of a field with a given name in a given entity.
 * This ensures that multiple instances of an entity do not duplicate this but can benefit from a shared singleton
 * to provide this metadata (and shared behaviour on a field).
 */
class Field {
public:
    Field(const string &name_) : name(name_) {}
    const string &Name() const { return name; }

private:
    string name;
};

class EntityClass {
public:
    EntityClass(const string &fqn_) : fqn(fqn_) {}
    virtual const string & FQN() const { return fqn; }
    virtual size_t FieldCount() const;
    virtual Field *GetField(size_t index) const;
    virtual Field *GetField(const string & name);

protected:
    virtual void AddField(Field *field);

private:
    string fqn = "";
    EntityClass *base = nullptr;
    vector<Field *> field_list;
    map<string, Field *> field_map;
};

template <typename ValueType>
class FieldValue {
public:
    FieldValue(const Field *const field) : field(field) {}
    virtual ~FieldValue() = 0;

    const ValueType &Get();

    void Set(const ValueType &value);

private:
    const Field *field;
};

class EntityBase {
public:
    EntityBase() { }
    virtual ~EntityBase();
    virtual size_t FieldCount() const;
    virtual Field *GetField(const string & name);
    virtual const list<shared_ptr<Field>> &Fields() const = 0;
    virtual const list<shared_ptr<Field>> &KeyFields() const = 0;

protected:
    virtual void SetField(const string &name, Field *field);
    EntityBase(EntityClass *eclass) : entity_class(eclass) { };

private:
    EntityClass *entity_class;
    unordered_map<string, Field*> fields_by_name;
};

template <typename ET, typename FT>
class FieldFactory {
public:
    using EntityType = enable_if_t<is_base_of<EntityClass, FT>::value, void>;
    using FieldType = enable_if_t<is_base_of<Field, FT>::value, void>;

    static FieldType *Create(string field_name);
};

template <typename E, enable_if_t<is_base_of<EntityClass, E>::value, nullptr_t> = nullptr>
class Entity : public EntityBase {
protected:
    template <typename FT, enable_if_t<is_base_of<Field, FT>::value, nullptr_t> = nullptr>
    using FieldFactoryType = FieldFactory<Entity<E>, FT>;

public:
    Entity(E *eclass) : EntityBase(eclass) { };
    E *GetClass() const { return entity_class; }

    template<typename ...FieldTypes>
    const Key<FieldTypes...> &GetKey() const;

    template<typename ...FieldTypes>
    void SetKey(const Key<FieldTypes...> &value);

    template <class F> // enable_if_t<is_base_of<Field, F>::value, void>
    F *GetField(const string &fieldname) { return (F *)GetField(fieldname); }
};

#endif

