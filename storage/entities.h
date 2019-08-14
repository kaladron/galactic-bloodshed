
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef ENTITIES_H
#define ENTITIES_H

#include "storage/values.h"

START_NS

class Entity : public MapValue {
public:
    Entity(Schema *schema);
    virtual ~Entity();

    Schema *GetSchema() const { return schema; }

    /** Get the value of the key fields corresponding to this Entity. */
    virtual const Value *GetKey() const;

    /** Sets the values of the key fields corresponding to this Entity. */
    virtual void SetKey(const Value &value);

protected:
    Schema *schema;
};

END_NS

#endif

