
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
    };

    struct Required {
        // A collection of fields that participate in uniqueness
        FieldPath field_path;
    };

    struct DefaultValue {
        FieldPath field_path;
        Value *value;
        bool onread;
    };

    struct ForeignKey {
        std::list<FieldPath> src_field_paths;
        std::list<FieldPath> dst_field_paths;
        const Schema *dst_schema;
    };

public:
    // Constructor for creating a uniqueness constraint
    Constraint(std::list<FieldPath> &field_paths);

    // Optional field constructor
    Constraint(const FieldPath &field_path);

    // Default value constraints
    Constraint(const FieldPath &field_path, Value *value, bool onread = true);

    // Foreign key cardinality constraints
    Constraint(const std::list<FieldPath> &src,
               const std::list<FieldPath> &dst, 
               const Schema *dst_schema);

    bool IsUniqueness() const { return tag == UNIQUE; }
    bool IsRequired() const { return tag == REQUIRED; }
    bool IsDefaultValue() const { return tag == DEFAULT_VALUE; }
    bool IsForeignKey() const { return tag == FOREIGN_KEY; }

    const Uniqueness &AsUniqueness() const { return uniqueness; }
    const Required &AsRequired() const { return required; }
    const ForeignKey &AsForeignKey() const { return foreign_key; }
    const DefaultValue &AsDefaultValue() const { return default_value; }

protected:
    Type tag;
    union {
        Uniqueness uniqueness;
        Required required;
        ForeignKey foreign_key;
        DefaultValue default_value;
    };
};

class Schema {
public:
    Schema(const std::string &name, Type *t, Type *kt);
    virtual ~Schema() { }

    /** Returns the name of the schema. */
    const std::string &Name() const { return name; }

    /** Returns the type of the key for this entity. */
    Type *KeyType() const { return key_type; }

    /** Returns the type of the entity. */
    Type *EntityType() const { return key_type; }

    /** Add a new constraint into the Schema. */
    void AddConstraint(const Constraint *c);
    const std::list<const Constraint *> &GetConstraints() const;

protected:
    std::string name;
    Type *entity_type;
    Type *key_type;
    std::list <const Constraint *> constraints;
};

END_NS

#endif

