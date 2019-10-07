
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef REGISTRY_H
#define REGISTRY_H

#include <variant>
#include "storage/fwds.h"

START_NS

class Registry {
public:
    virtual ~Registry();

    Registry *Add(StrongType t);
    Registry *Add(shared_ptr<Schema> t);
    Registry *Add(const std::list<StrongType> &types);
    Registry *Add(const std::list<shared_ptr<Schema>> &schemas);

    StrongType GetType(const string &fqn) const;
    shared_ptr<Schema> GetSchema(const string &fqn) const;

protected:
    mutable map<string, StrongType> types;
    mutable map<string, shared_ptr<Schema>> schemas;
};

extern std::list<StrongType> DefaultCTypes();

END_NS

#endif

