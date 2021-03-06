
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include "storage/values.h"

START_NS

class Schema {
public:
    Schema(const std::string &fqn, StrongType t, std::initializer_list<FieldPath> key_fields);
    Schema(const std::string &fqn, StrongType t, const vector<FieldPath> &key_fields);
    virtual ~Schema() { }

    /** Returns the fqn of the schema. */
    const std::string &FQN() const { return fqn; }

    /** Returns the type of the key for this entity. */
    StrongType KeyType() const;

    /** Returns the type of the entity. */
    StrongType EntityType() const { return entity_type; }

    /**
     * Returns the value of a particular value.
     */
    StrongValue GetKey(const Value &value) const;

    /**
     * Sets the key fields of a particular value.
     */
    void SetKey(const Value &value, const Value &key);

    const vector<FieldPath> &KeyFields() const { return key_fields; }

    /** Add a new constraint into the Schema. */
    Schema *AddConstraint(Constraint *c);
    const std::list<Constraint *> &GetConstraints() const;

protected:
    std::string fqn;
    StrongType entity_type;
    vector<FieldPath> key_fields;
    mutable StrongType key_type;
    std::list <Constraint *> constraints;
};

/**
 * Describes a constraint in a Schema.
 */
class Constraint {
public:
    enum Type {
        UNIQUE,
        REQUIRED,
        DEFAULT_VALUE,
        FOREIGN_KEY,
    };

    struct Uniqueness {
        // A collection of fields that participate in uniqueness
        // Upto the storage on how this is implementated (if it can be)
        std::list<FieldPath> field_paths;

        Uniqueness() { }
        Uniqueness(std::list<FieldPath> &field_paths);
    };

    struct Required {
        // A collection of fields that participate in uniqueness
        FieldPath field_path;

        Required() { }
        Required(const FieldPath &field_path);
    };

    struct DefaultValue {
        FieldPath field_path;
        StrongValue value;
        bool onread;

        DefaultValue() { }
        DefaultValue(const FieldPath &field_path, StrongValue value, bool onread = true);
    };

    struct ForeignKey {
        std::list<FieldPath> src_field_paths;
        std::list<FieldPath> dst_field_paths;
        const Schema *dst_schema;

        ForeignKey() { }
        ForeignKey(const std::list<FieldPath> &src,
                   const std::list<FieldPath> &dst,
                   const Schema *dst_schema);
    };

public:
    // Constructor for creating a uniqueness constraint
    Constraint(const Uniqueness &uniqueness);

    // Optional field constructor
    Constraint(const Required &required_);

    // Default value constraints
    Constraint(const DefaultValue &defval);

    // Foreign key cardinality constraints
    Constraint(const ForeignKey &fkey);

    bool IsUniqueness() const { return tag == UNIQUE; }
    bool IsRequired() const { return tag == REQUIRED; }
    bool IsDefaultValue() const { return tag == DEFAULT_VALUE; }
    bool IsForeignKey() const { return tag == FOREIGN_KEY; }

    const auto &AsUniqueness() const { return std::get<Uniqueness>(uniqueness); }
    const auto &AsRequired() const { return std::get<Required>(required); }
    const auto &AsForeignKey() const { return std::get<ForeignKey>(foreign_key); }
    const auto &AsDefaultValue() const { return std::get<DefaultValue>(default_value); }

protected:
    Type tag;
    std::variant<Uniqueness, Required, ForeignKey, DefaultValue> uniqueness, required, foreign_key, default_value;
};

END_NS

#endif

