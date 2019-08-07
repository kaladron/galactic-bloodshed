

// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/stores/schema.h"

Schema::Schema(const string &name_, Type *t, Type *kt) : 
    name(name_), entity_type(t), key_type(kt) {
    assert(t->Tag() == Type::RECORD && "Schemas can only be record types.");
}

