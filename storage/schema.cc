

// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

/**
 * Create a new constraint identifying uniqueness across a set
 * of field paths.
 */
Constraint::Uniqueness::Uniqueness(list<FieldPath> &fps) : field_paths(fps) {
}

/**
 * Create a new constraint declaring the required nature of a 
 * given field path.
 */
Constraint::Required::Required(const FieldPath &fp) : field_path(fp) {
}

/**
 * Attach a default value to a field_path to be applied when empty
 * either on read or write depending on the "onread" paraemter.
 */
Constraint::DefaultValue::DefaultValue(const FieldPath &fp,
                                       StrongValue val,
                                       bool onread_) : field_path(fp), value(val), onread(onread_) {
}

/**
 * Creates a foreign key constraint between a field in the source
 * type to the field path in a destination type.
 */
Constraint::ForeignKey::ForeignKey(
        const list<FieldPath> &src,
        const list<FieldPath> &dst, 
        const Schema *schema) {
    assert(src.size() > 0 && "Foreign key constraint must have at least one column");
    assert(src.size() == dst.size() && "Foreign key source and dest columns must be of same size.");
    src_field_paths = src;
    dst_field_paths = dst;
    dst_schema = schema;
}

/**
 * Create a new constraint identifying uniqueness across a set
 * of field paths.
 */
Constraint::Constraint(const Uniqueness &uniqueness_) : tag(Type::UNIQUE) {
    uniqueness = uniqueness_;
}

/**
 * Create a new constraint declaring the required nature of a 
 * given field path.
 */
Constraint::Constraint(const Required &required_) : tag(Type::REQUIRED), required(required_) {
}

/**
 * Attach a default value to a field_path to be applied when empty
 * either on read or write depending on the "onread" paraemter.
 */
Constraint::Constraint(const DefaultValue &defval) : tag(DEFAULT_VALUE), default_value(defval) {
}

/**
 * Creates a foreign key constraint between a field in the source
 * type to the field path in a destination type.
 */
Constraint::Constraint(const ForeignKey &fkey) : tag(FOREIGN_KEY), foreign_key(fkey) {
}

/**
 * Create a new schema with a given fqn and the underlying record type.
 */
Schema::Schema(const string &fqn_, StrongType t, const vector<FieldPath> &kf) 
    : fqn(fqn_), entity_type(t), key_fields(kf) {
    assert(t->IsProductType() && "Schemas can only be named product types.");
}

Schema::Schema(const std::string &f, StrongType t, std::initializer_list<FieldPath> kf) 
    : Schema(f, t, vector<FieldPath>(kf)) {
}

StrongType Schema::KeyType() const {
    if (!key_type) {
        // evaluate it
    }
    return key_type;
}

/**
 * Add a new constraint to apply.  Ownership of Constraint is transferred to
 * the Schema.
 */
Schema *Schema::AddConstraint(Constraint *c) {
    constraints.push_back(c);
    return this;
}

/**
 * Get the list of constraints applying to this schema.
 */
const list<Constraint *> &Schema::GetConstraints() const { return constraints; }

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
