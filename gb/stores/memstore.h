
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MEMSTORE_H
#define MEMSTORE_H

#include "store.h"

class MemTable : public Table {
public:
    MemTable(const Schema *t) { }
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);

private:
    std::map<const Value *, Entity *> entries;
};

class MemStore : public Store<MemTable> {
public:
    MemStore();
    virtual shared_ptr<MemTable> GetTable(const Schema *t);

private:
    map<const Schema *, shared_ptr<MemTable>> tables;
};

#endif
