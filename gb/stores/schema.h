
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Classes and routines to abstract persistance.
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include <cstdlib>
#include <memory>
#include <utility>
#include <tuple>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <type_traits>
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include "gb/stores/values.h"

using namespace std;

/**
 * Describes a constraint in a Schema.
 */
class Constraint {
public:
    enum Type {
        UNIQUE,
        OPTIONAL,
        DEFAULT_VALUE,
        FOREIGN_KEY,
    };
    Type tag;

public:
    // Constructor for creating a uniqueness constraint
    Constraint(list<FieldPath> &field_paths);

    // Optional field constructor
    Constraint(const FieldPath &field_path);

    // Default value constraints
    Constraint(const FieldPath &field_path, Value *value, bool onread = true);

    // Foreign key cardinality constraints
    Constraint(const FieldPath &src, const Type *dst_type,
               const FieldPath &dst, bool many2many = false);

protected:
    union {
        struct {
            // A collection of fields that participate in uniqueness
            // Upto the storage on how this is implementated (if it can be)
            list<FieldPath> field_paths;
        } uniqueness;

        struct {
            // A collection of fields that participate in uniqueness
            FieldPath field_path;
        } optionality;

        struct {
            FieldPath src_field_path;
            const Type *dst_type;
            FieldPath dst_field_path;
            bool many2many;
        } foreign_key;

        struct {
            FieldPath field_path;
            Value *value;
            bool onread;
        } default_value;
    };
};

class Schema {
public:
    Schema(const string &name, Type *t, Type *kt);
    virtual ~Schema() { }

    /** Returns the name of the schema. */
    const string &Name() const { return name; }

    /** Returns the type of the key for this entity. */
    Type *KeyType() const { return key_type; }

    /** Returns the type of the entity. */
    Type *EntityType() const { return key_type; }

    /** Add a new constraint into the Schema. */
    void AddConstraint(const Constraint *c);
    const list<const Constraint *> &GetConstraints() const;

protected:
    string name;
    Type *entity_type;
    Type *key_type;
    list <const Constraint *> constraints;
};

#endif

