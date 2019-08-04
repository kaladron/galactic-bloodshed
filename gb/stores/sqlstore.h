
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MEMSTORE_H
#define MEMSTORE_H

#include <map>
#include "store.h"

class SQLStore : public Store {
public:
    template <class E, typename ...Fs>
    const Table<E, Fs...> &GetTable();
};

template <class E, typename ...Fs>
class SQLTable : public Table<E, Fs...> {
public:
    using EntityType = Table<E, Fs...>::EntityType;
    void Get(EntityType &entity, const Key<Fs...> &key);
    void Put(EntityType &entity);
    void Delete(const Key<Fs...> &key);

private:
};

template <class E, typename ...Fs>
void SQLTable<E, Fs...>::Get(EntityType &entity, const Key<Fs...> &key) {
}

template <class E, typename ...Fs>
void SQLTable<E, Fs...>::Put(EntityType &entity) {
}

template <class E, typename ...Fs>
void SQLTable<E, Fs...>::Delete(const Key<Fs...> &key) {
}

#endif

