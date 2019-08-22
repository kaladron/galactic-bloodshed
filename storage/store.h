
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
    virtual ~Collection() { }
    bool Get(const Value &key, Value &result);
    void Put(Value &entity);
    void DeleteByKey(const Value &key);

    void Delete(const Value &key);
    void Put(const std::list<Value *> &values);

    void Delete(const std::list<const Value *> &keys) {
        for (auto key : keys) {
            Delete(*key);
        }
    }

protected:
    const Schema *schema;
};

template<class CollectionType, typename ValueType>
class TypedCollection : protected CollectionType {
public:
    TypedCollection(const Schema *schema_) : CollectionType(schema_) { }
    virtual ~TypedCollection() { }
    bool Get(const ValueType &key, ValueType &result) {
        return CollectionType::Get(key, result);
    }
    void Put(ValueType &entity) {
        CollectionType::Put(entity);
    }
    void DeleteByKey(const ValueType &key) {
        CollectionType::DeleteByKey(key);
    }

    void Delete(const ValueType &entity) {
        CollectionType::Delete(entity);
    }

    void Put(const std::list<ValueType *> &values) {
        CollectionType::Put(values);
    }

    void Delete(const std::list<const Value *> &keys) {
        CollectionType::Delete(keys);
    }
};

template <typename CollectionType>
class Store {
public:
    virtual std::shared_ptr<CollectionType> GetCollection(const Schema *t);
};

END_NS

#endif
