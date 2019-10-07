
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

using std::make_shared;

list<StrongType> DefaultCTypes() {
    return list({
        make_shared<Type>("char"),
        make_shared<Type>("bool"),
        make_shared<Type>("int8"),
        make_shared<Type>("int16"),
        make_shared<Type>("int32"),
        make_shared<Type>("int64"),
        make_shared<Type>("uint8"),
        make_shared<Type>("uint16"),
        make_shared<Type>("uint32"),
        make_shared<Type>("uint64"),
        make_shared<Type>("float"),
        make_shared<Type>("double"),
        make_shared<Type>("string"),
    });
}

Registry::~Registry() {
}

/**
 * Adds a type to the regitry.  Adding a type transfers ownership
 * and its lifecycle is now bound to the registry.
 */
Registry *Registry::Add(StrongType t) {
    auto fqn = t->FQN();
    assert (types.find(fqn) == types.end() && "Type already exists");
    types.emplace(fqn, t);
    return this;
}

/**
 * Adds a schema to the regitry.  Adding a schema transfers ownership
 * and its lifecycle is now bound to the registry.
 */
Registry *Registry::Add(shared_ptr<Schema> s) {
    auto fqn = s->FQN();
    assert (schemas.find(fqn) == schemas.end() && "Schema already exists");
    schemas.emplace(fqn, s);
    return this;
}

Registry *Registry::Add(const std::list<StrongType> &newtypes) {
    for (auto t : newtypes) {
        Add(t);
    }
    return this;
}

Registry *Registry::Add(const std::list<shared_ptr<Schema>> &newschemas) {
    for (auto s : newschemas) {
        Add(s);
    }
    return this;
}

StrongType Registry::GetType(const string &fqn) const {
    auto it = types.find(fqn);
    if (it != types.end()) {
        return it->second;
    }
    return nullptr;
}

shared_ptr<Schema> Registry::GetSchema(const string &fqn) const {
    auto it = schemas.find(fqn);
    if (it != schemas.end()) {
        return it->second;
    }
    return nullptr;
}

END_NS
