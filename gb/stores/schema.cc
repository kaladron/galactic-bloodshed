

// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/stores/schema.h"

Schema::Schema(const string &name_, Type *t, Type *kt) : 
    name(name_), entity_type(t), key_type(kt) {
    assert(t->Tag() == Type::RECORD && "Schemas can only be record types.");
}
// Constructor for creating a uniqueness constraint
Constraint::Constraint(list<FieldPath> &field_paths) : tag(Type::UNIQUE) {
    uniqueness.field_paths = field_paths;
}

// Optional field constructor
Constraint::Constraint(const FieldPath &fields) : tag(Type::OPTIONAL) {
    optionality.field_path = fields;
}

// Default value constraints
Constraint::Constraint(const FieldPath &field_path, Value *value, bool onread) : tag(DEFAULT_VALUE) {
    default_value.field_path = field_path;
    default_value.value = value;
    default_value.onread = onread;
}

// Foreign key cardinality constraints
Constraint::Constraint(const FieldPath &src, const FieldPath &dst, bool many2many) : tag(FOREIGN_KEY) {
    foreign_key.src_field_path = src;
    foreign_key.dst_field_path = dst;
    foreign_key.many2many = many2many;
}
