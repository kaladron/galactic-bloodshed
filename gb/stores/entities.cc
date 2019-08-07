
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/stores/entities.h"

Schema::Schema(const string &name_, Type *t, Type *kt) : 
    name(name_), entity_type(t), key_type(kt) {
    assert(t->Tag() == Type::RECORD && "Schemas can only be record types.");
}

Entity::Entity(Schema *s) : MapValue(s->EntityType()), schema(s) {
}

Entity::~Entity() {
}

/** Get the value of the key fields corresponding to this Entity. */
const Value *Entity::GetKey() const {
    return nullptr;
}

/** Sets the values of the key fields corresponding to this Entity. */
void Entity::SetKey(const Value &value) {
}
