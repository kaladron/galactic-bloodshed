
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef STORE_H
#define STORE_H

#include "storage/fwds.h"

START_NS

/**
 * The Collection is our storage abstraction.  Collection contains of a collection of entities of a given type.
 */
class Collection {
public:
    Collection(const Schema *schema_) : schema(schema_) { }
    virtual ~Collection();
    virtual Value *Get(const Value &key) = 0;
    virtual bool Put(Value &entity) = 0;
    virtual bool DeleteByKey(const Value &key) = 0;

    virtual bool Put(Value *entity);
    virtual bool Put(const std::list<Value *> &values);
    virtual bool Delete(const Value &key);
    virtual bool Delete(const std::list<const Value *> &keys);

protected:
    const Schema *schema;
};

template<class CollectionType, typename ValueType>
class TypedCollection : protected CollectionType {
public:
    TypedCollection(const Schema *schema_) : CollectionType(schema_) { }
    virtual ~TypedCollection() { }
    virtual bool Get(const ValueType &key, ValueType &result) {
        return CollectionType::Get(key, result);
    }
    virtual void Put(ValueType &entity) {
        CollectionType::Put(entity);
    }
    virtual void DeleteByKey(const ValueType &key) {
        CollectionType::DeleteByKey(key);
    }

    virtual void Delete(const ValueType &entity) {
        CollectionType::Delete(entity);
    }

    virtual void Put(const std::list<ValueType *> &values) {
        CollectionType::Put(values);
    }

    virtual void Delete(const std::list<const Value *> &keys) {
        CollectionType::Delete(keys);
    }
};

template <typename CollectionType>
class Store {
public:
    virtual ~Store() { }
    virtual std::shared_ptr<CollectionType> GetCollection(const Schema *t) = 0;
};

END_NS

#endif
