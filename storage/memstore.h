
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MEMSTORE_H
#define MEMSTORE_H

#include "storage/store.h"

START_NS

class MemCollection : public Collection {
public:
    MemCollection(const Schema *t);
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);

private:
    const Schema *schema;
    std::map<const Value *, Entity *> entries;
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
