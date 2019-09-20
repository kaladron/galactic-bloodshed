
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

    Registry *Add(Type *t);
    Registry *Add(Schema *t);
    Registry *Add(std::list<Type *> types);
    Registry *Add(std::list<Schema *> schemas);

    const Type *GetType(const string &fqn) const;
    const Schema *GetSchema(const string &fqn) const;

protected:
    mutable map<string, unique_ptr<Type>> types;
    mutable map<string, unique_ptr<Schema>> schemas;
};

extern std::list<Type *> DefaultCTypes();

END_NS

#endif

