

// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

/**
 * Create a new constraint identifying uniqueness across a set
 * of field paths.
 */
Constraint::Constraint(list<FieldPath> &field_paths) : tag(Type::UNIQUE) {
	std::get<Uniqueness>(uniqueness).field_paths = field_paths;
}

/**
 * Create a new constraint declaring the required nature of a 
 * given field path.
 */
Constraint::Constraint(const FieldPath &fields) : tag(Type::REQUIRED) {
	std::get<Required>(required).field_path = fields;
}

/**
 * Attach a default value to a field_path to be applied when empty
 * either on read or write depending on the "onread" paraemter.
 */
Constraint::Constraint(const FieldPath &field_path, StrongValue value, bool onread) : tag(DEFAULT_VALUE) {
	std::get<DefaultValue>(default_value).field_path = field_path;
	std::get<DefaultValue>(default_value).value = value;
	std::get<DefaultValue>(default_value).onread = onread;
}

/**
 * Creates a foreign key constraint between a field in the source
 * type to the field path in a destination type.
 */
Constraint::Constraint(const list<FieldPath> &src,
                      const list<FieldPath> &dst, 
                      const Schema *dst_schema) : tag(FOREIGN_KEY) {
    assert(src.size() > 0 && "Foreign key constraint must have at least one column");
    assert(src.size() == dst.size() && "Foreign key source and dest columns must be of same size.");
    std::get<ForeignKey>(foreign_key).src_field_paths = src;
    std::get<ForeignKey>(foreign_key).dst_field_paths = dst;
    std::get<ForeignKey>(foreign_key).dst_schema = dst_schema;
}

/**
 * Create a new schema with a given fqn and the underlying record type.
 */
Schema::Schema(const string &fqn_, const Type *t, const vector<FieldPath> &kf) 
    : fqn(fqn_), entity_type(t), key_fields(kf) {
    assert(t->IsRecord() && "Schemas can only be record types.");
}

const Type *Schema::KeyType() const {
    if (key_type == nullptr) {
        // evaluate it
    }
    return key_type;
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

/**
 * Returns the value of a particular value.
 */
StrongValue Schema::GetKey(const Value & /*value*/) const {
    return StrongValue();
}

/**
 * Sets the key fields of a particular value.
 */
void Schema::SetKey(const Value & /*value*/, const Value & /*key*/) {
}

END_NS
