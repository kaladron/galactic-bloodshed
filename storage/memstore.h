
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
    bool Get(const Value &key, Value &result);
    void Put(Value &entity);
    void DeleteByKey(const Value &key);

private:
    std::map<const Value *, Value *> entries;
};

class MemStore : public Store<MemCollection> {
public:
    MemStore();
    virtual std::shared_ptr<MemCollection> GetCollection(const Schema *t);

private:
    std::map<const Schema *, std::shared_ptr<MemCollection>> tables;
};

END_NS

#endif
