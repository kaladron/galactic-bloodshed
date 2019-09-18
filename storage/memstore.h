
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MEMSTORE_H
#define MEMSTORE_H

#include "storage/store.h"

START_NS

class MemCollection : public Collection {
public:
    MemCollection(const Schema *schema_);
    virtual StrongValue Get(StrongValue key);
    virtual bool Put(StrongValue entity);
    virtual bool Delete(StrongValue key);

private:
    std::map<StrongValue, StrongValue> entries;
};

class MemStore : public Store {
public:
    MemStore();
    virtual shared_ptr<Collection> GetCollection(const Schema *t);

private:
    std::map<const Schema *, std::shared_ptr<MemCollection>> tables;
};

END_NS

#endif
