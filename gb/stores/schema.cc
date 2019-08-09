

// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/stores/schema.h"

/**
 * Create a new constraint identifying uniqueness across a set
 * of field paths.
 */
Constraint::Constraint(list<FieldPath> &field_paths) : tag(Type::UNIQUE) {
    uniqueness.field_paths = field_paths;
}

/**
 * Create a new constraint declaring the required nature of a 
 * given field path.
 */
Constraint::Constraint(const FieldPath &fields) : tag(Type::OPTIONAL) {
    optionality.field_path = fields;
}

/**
 * Attach a default value to a field_path to be applied when empty
 * either on read or write depending on the "onread" paraemter.
 */
Constraint::Constraint(const FieldPath &field_path, Value *value, bool onread) : tag(DEFAULT_VALUE) {
    default_value.field_path = field_path;
    default_value.value = value;
    default_value.onread = onread;
}

/**
 * Creates a foreign key constraint between a field in the source
 * type to the field path in a destination type.
 * Additionaly many2many specifies the cardinality of this relationship.
 */
Constraint::Constraint(const FieldPath &src, const Type *dst_type,
                       const FieldPath &dst, bool many2many) : tag(FOREIGN_KEY) {
    foreign_key.src_field_path = src;
    foreign_key.dst_type = dst_type;
    foreign_key.dst_field_path = dst;
    foreign_key.many2many = many2many;
}

/**
 * Create a new schema with a given name and the underlying record type.
 */
Schema::Schema(const string &name_, Type *t, Type *kt) : 
    name(name_), entity_type(t), key_type(kt) {
    assert(t->Tag() == Type::RECORD && "Schemas can only be record types.");
}

/**
 * Add a new (readonly) constraint to apply.
 */
void Schema::AddConstraint(const Constraint *c) {
    constraints.push_back(c);
}

/**
 * Get the list of constraints applying to this schema.
 */
const list<const Constraint *> &Schema::GetConstraints() const {
    return constraints;
}
