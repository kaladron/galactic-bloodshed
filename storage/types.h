
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TYPES_H
#define TYPES_H

#include "storage/fwds.h"

using namespace std;

START_NS

using NameTypePair = pair<string, const Type *>;
using NameTypeVector = vector<NameTypePair>;
using TypeVector = vector<const Type *>;

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

    Type(const string &fqn);
    Type(const string &fqn, const TypeVector &args);
    Type(const string &fqn, const NameTypeVector &fields, bool is_product_type = true);
    ~Type();

    int Compare(const Type &another) const;

    void SetData(const TypeVector &args);
    void SetData(const NameTypeVector &fields, bool is_product_type = true);

    // Access children
    void AddChild(const Type *child, const string &name = "");
    size_t ChildCount() const;
    const Type *GetChild(const string &name) const;
    NameTypePair GetChild(size_t index) const;
    const string &FQN() const { return fqn; }

    bool IsTypeFun() const { return type_tag == TYPE_FUN; }
    bool IsRecord() const { return type_tag == RECORD; }
    bool IsUnion() const { return type_tag == UNION; }

protected:
    void Clear();

private:
    /**
     * FQN of this type
     */
    const string fqn;

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

class DefaultTypes {
public:
    static const Type *CharType();
    static const Type *BoolType();
    static const Type *IntType();
    static const Type *FloatType();
    static const Type *LongType();
    static const Type *DoubleType();
    static const Type *StringType();
};

END_NS

// Some macros to make creation of types easier

#if 0
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
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

#endif

