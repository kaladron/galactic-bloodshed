
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef ENTITIES_H
#define ENTITIES_H

#include <cstdlib>
#include <memory>
#include <utility>
#include <tuple>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <type_traits>
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include "gb/stores/values.h"

using namespace std;

class Entity : public MapValue {
public:
    Entity(Type *t, Type *kt);
    virtual ~Entity();

    /** Returns the type of the key for this entity. */
    Type *GetKeyType() const { return key_type; }

    /** Get the value of the key fields corresponding to this Entity. */
    virtual const Value *GetKey() const;

    /** Sets the values of the key fields corresponding to this Entity. */
    virtual void SetKey(const Value &value);

protected:
    Type *key_type;
};

#endif

