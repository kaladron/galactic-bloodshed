
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MEMSTORE_H
#define MEMSTORE_H

#include <map>
#include "store.h"

class MemStore : public Store {
public:
    template <typename E, typename ...Fs>
    const Table<E, Fs...> &GetTable();
};

template <typename E, typename ...Fs>
class MemTable : public Table<E, Fs...> {
public:
    using EntityType = Table<E, Fs...>::EntityType;
    void Get(EntityType &entity, const Key<Fs...> &key) { return entries[key]; }
    void Put(EntityType &entity) { entries[entity.GetKey()] = entity; }
    void Delete(const Key<Fs...> &key) { entries.erase(key); }

private:
    std::map<Key<Fs...>, EntityType> entries;
};

#endif
