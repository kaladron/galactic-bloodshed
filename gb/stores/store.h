
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef STORE_H
#define STORE_H

#include "entities.h"

using namespace std;

/**
 * The Table is our storage abstraction.  Table contains of a collection of entities of a given type.
 */
template <class E, typename ...Fs>
class Table {
public:
    using EntityType = Entity<enable_if_t<is_base_of<EntityClass, E>::value, void>>;
    void Get(EntityType &entity, const Key<Fs...> &key);
    void Put(EntityType &entity);
    void Delete(const Key<Fs...> &key);

    void Put(const list<EntityType> &entities) {
        for (auto entity : entities) {
            Put(entity);
        }
    }

    void Delete(const EntityType &entity) {
        Delete(entity.GetKey());
    }

    void Delete(const list<Key<EntityType>> &keys) {
        for (auto key : keys) {
            Delete(key);
        }
    }

    void Delete(const list<EntityType> &entities) {
        for (auto entity : entities) {
            Put(entity);
        }
    }
};

class Store {
    template <typename E, typename ...Fs>
    const Table<E, Fs...> &GetTable();
};

#endif
