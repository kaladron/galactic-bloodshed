
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLSTORE_H
#define SQLSTORE_H

#include "store.h"

class SQLStore : public Store {
public:
    const Table *GetTable(Type *t);
};

class SQLTable : public Table {
public:
    void Get(Entity &entity, const Value &key) { entity = entries[&key]; }
    void Put(Entity &entity) { entries[entity.GetKey()] = entity; }
    void Delete(const Value &key) { entries.erase(key); }

private:
    std::map<const Value *, Entity &> entries;
};

#endif
