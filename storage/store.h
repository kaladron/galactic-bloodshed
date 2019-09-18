
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
    virtual StrongValue Get(StrongValue key) = 0;
    virtual bool Put(StrongValue entity) = 0;
    virtual bool Delete(StrongValue key) = 0;

    virtual bool Put(const std::list<StrongValue> &values);
    virtual bool Delete(const std::list<StrongValue> &keys);

protected:
    const Schema *schema;
};

class Store {
public:
    virtual ~Store() { }
    virtual std::shared_ptr<Collection> GetCollection(const Schema *t) = 0;
};

END_NS

#endif
