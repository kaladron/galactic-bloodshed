
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef STORE_H
#define STORE_H

#include "storage/fwds.h"

START_NS

/**
 * The Collection is our storage abstraction.  Collection contains of a collection of entities of a given type.
 */
class Collection {
public:
    Collection() { }
    virtual ~Collection() { }
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);

    void Put(Entity *entity);
    void Put(const std::list<Entity*> &entities);

    void Delete(const Value *key);
    void Delete(const Entity *entity);
    void Delete(const Entity &entity) { Delete(&entity); }

    void Delete(const std::list<const Value *> &keys) {
        for (auto key : keys) {
            Delete(key);
        }
    }

    void Delete(const std::list<Entity *> &entities) {
        for (auto entity : entities) {
            Delete(entity);
        }
    }
};

template <typename CollectionType>
class Store {
public:
    virtual std::shared_ptr<CollectionType> GetCollection(const Schema *t);
};

END_NS

#endif
