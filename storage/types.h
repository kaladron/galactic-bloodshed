
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TYPES_H
#define TYPES_H

#include "storage/fwds.h"

using namespace std;

START_NS

using NameTypePair = pair<const string, Type *>;
using NameTypeVector = vector<NameTypePair>;
using TypeVector = vector<Type *>;

class FieldPath : public vector<string> {
public:
    FieldPath() { }
    FieldPath(const string &input, const string &delim = "/");
    FieldPath(const vector<string> &another);
    FieldPath push(const string &subfield);
    string join(const string &delim = "/") const;
};

class Type {
public:
    enum TypeTag {
        // Functor types
        RECORD,
        UNION,
        TYPE_FUN
    };

    Type(const string &name);
    Type(const string &name, const TypeVector &args);
    Type(const string &name, const NameTypeVector &fields, bool is_product_type = true);
    ~Type();

    int Compare(const Type &another) const;

    void SetData(const TypeVector &args);
    void SetData(const NameTypeVector &fields, bool is_product_type = true);

    // Access children
    void AddChild(Type *child, const string &name = "");
    size_t ChildCount() const;
    Type *GetChild(const string &name) const;
    NameTypePair GetChild(size_t index) const;
    const string &Name() const { return name; }

    const bool IsTypeFun() const { return type_tag == TYPE_FUN; }
    const bool IsRecord() const { return type_tag == RECORD; }
    const bool IsUnion() const { return type_tag == UNION; }

protected:
    void Clear();

private:
    /**
     * Name of this type
     */
    const string name;

    /**
     * The tag of this type to identify the payload union.
     */
    TypeTag type_tag;

    /**
     * Children of this type.
     * For TYPE_FUN the name part of the pair will be empty!!!
     */
    vector<string> child_names;
    TypeVector child_types;
};

// SOME DEFAULT TYPES
extern const Type *CharType;
extern const Type *BoolType;
extern const Type *IntType;
extern const Type *FloatType;
extern const Type *LongType;
extern const Type *DoubleType;

END_NS

// Some macros to make creation of types easier

#define REGISTER_FIELD(field_name, field_type)           \
    RegisterField(#field_name, (Field *)field_name);

#define REGISTER_FIELD_MACRO(r, data, elem)          \
    REGISTER_FIELD(BOOST_PP_TUPLE_ELEM(0, elem), BOOST_PP_TUPLE_ELEM(1, elem))

#define DEFINE_FIELD(field_name, field_type)           \
    const field_type* field_name = RegisterField<field_type>(#field_name);

#define DEFINE_FIELD_MACRO(r, data, elem)          \
    DEFINE_FIELD(BOOST_PP_TUPLE_ELEM(0, elem), BOOST_PP_TUPLE_ELEM(1, elem))

#define DEFINE_RECORD(record_name, fields)                      \
    new Type(record_name, new NameTypeVector() {                \
        BOOST_PP_SEQ_FOR_EACH(DEFINE_FIELD_MACRO, _, fields)    \
    }, true);

#endif

