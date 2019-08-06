
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/stores/entities.h"

Entity::Entity(Type *t, Type *kt) : MapValue(t), key_type(kt) {
    assert(t->Tag() == Type::RECORD && "Entities types can only be records.");
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
