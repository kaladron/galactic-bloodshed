
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
class Table {
public:
    Table() { }
    virtual ~Table() { }
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);

    void Put(Entity *entity) { if (entity) Put(*entity); }
    void Put(const list<Entity*> &entities) {
        for (auto entity : entities) {
            Put(entity);
        }
    }

    void Delete(const Value *key) { Delete(*key); }
    void Delete(const Entity *entity) { Delete(entity->GetKey()); }
    void Delete(const Entity &entity) { Delete(&entity); }

    void Delete(const list<const Value *> &keys) {
        for (auto key : keys) {
            Delete(key);
        }
    }

    void Delete(const list<Entity *> &entities) {
        for (auto entity : entities) {
            Delete(entity);
        }
    }
};

template <typename TableType>
class Store {
public:
    virtual shared_ptr<TableType> GetTable(const Schema *t);
};

#endif
