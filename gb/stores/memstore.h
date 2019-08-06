
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MEMSTORE_H
#define MEMSTORE_H

#include "store.h"

class MemStore : public Store {
public:
    const Table *GetTable(Type *t);
};

class MemTable : public Table {
public:
    Entity *Get(const Value &key) { return entries[&key]; }
    void Put(Entity &entity) { entries[entity.GetKey()] = &entity; }
    void Delete(const Value &key) { entries.erase(&key); }

private:
    std::map<const Value *, Entity *> entries;
};

#endif
