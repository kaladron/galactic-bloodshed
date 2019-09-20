
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

list<Type *> DefaultCTypes() {
    return {
        new Type("char"),
        new Type("bool"),
        new Type("int8"),
        new Type("int16"),
        new Type("int32"),
        new Type("int64"),
        new Type("uint8"),
        new Type("uint16"),
        new Type("uint32"),
        new Type("uint64"),
        new Type("float"),
        new Type("double"),
        new Type("string"),
    };
}

Registry::~Registry() {
}

/**
 * Adds a type to the regitry.  Adding a type transfers ownership
 * and its lifecycle is now bound to the registry.
 */
Registry *Registry::Add(Type *t) {
    auto fqn = t->FQN();
    assert (types.find(fqn) == types.end() && "Type already exists");
    unique_ptr<Type> ut(t);
    types.emplace(fqn, std::move(ut));
    return this;
}

Registry *Registry::Add(std::list<Type *> newtypes) {
    for (auto t : newtypes) {
        Add(t);
    }
    return this;
}

Registry *Registry::Add(std::list<Schema *> newschemas) {
    for (auto s : newschemas) {
        Add(s);
    }
    return this;
}

/**
 * Adds a schema to the regitry.  Adding a schema transfers ownership
 * and its lifecycle is now bound to the registry.
 */
Registry *Registry::Add(Schema *s) {
    auto fqn = s->FQN();
    assert (schemas.find(fqn) == schemas.end() && "Schema already exists");
    unique_ptr<Schema> us(s);
    schemas.emplace(fqn, std::move(us));
    return this;
}

const Type *Registry::GetType(const string &fqn) const {
    auto it = types.find(fqn);
    if (it != types.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Schema *Registry::GetSchema(const string &fqn) const {
    auto it = schemas.find(fqn);
    if (it != schemas.end()) {
        return it->second.get();
    }
    return nullptr;
}

END_NS
