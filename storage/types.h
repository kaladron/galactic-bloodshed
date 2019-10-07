
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TYPES_H
#define TYPES_H

#include "storage/fwds.h"

START_NS

using NameTypePair = pair<std::string, StrongType>;
using NameTypeVector = vector<NameTypePair>;
using TypeVector = vector<StrongType>;

class FieldPath : public StringVector {
public:
    FieldPath() { }
    FieldPath(const string &input, const string &delim = "/");
    FieldPath(const StringVector &another);
    FieldPath push(const string &subfield);
    string join(const string &delim = "/") const;
};

class Type {
public:

public:
    enum Tag {
        // Functor types
        PRODUCT_TYPE,
        SUM_TYPE,
        TYPE_FUN,
        REF_TYPE
    };
    class TypeContainer {
        bool is_named = false;
        std::vector<std::pair<string, WeakType>> children;
        // StringVector child_names = {};
        // vector<WeakType> child_types = {};

    public:
        TypeContainer() { }
        TypeContainer(const TypeContainer &);
        TypeContainer(const TypeVector &args);
        TypeContainer(const NameTypeVector &fields);
        TypeContainer(std::initializer_list<StrongType> types);
        TypeContainer(std::initializer_list<NameTypePair> fields);
        virtual ~TypeContainer();

        int Compare(const TypeContainer &another) const;
        void Clear();
        void SetData(const TypeVector &args);
        void SetData(const NameTypeVector &fields);
        void AddChild(StrongType child, const string &name = "");
        size_t ChildCount() const;
        WeakType GetChild(const string &name) const;
        NameTypePair GetChild(size_t index) const;
    };

    struct ProductType : public TypeContainer {
        using TypeContainer::TypeContainer;
    };

    struct SumType : public TypeContainer {
        using TypeContainer::TypeContainer;
    };

    struct TypeFun : public TypeContainer {
        using TypeContainer::TypeContainer;
    };

    struct RefType {
        WeakType target_type;
        RefType(StrongType target_type);
        int Compare(const RefType &another) const;
    };

    Type(const string &fqn);
    Type(const string &fqn, const ProductType &prod_type);
    Type(const string &fqn, const SumType &sum_type);
    Type(const string &fqn, const TypeFun &typefun);
    Type(const string &fqn, const RefType &reftype);
    ~Type();

    int Compare(const Type &another) const;
    const string &FQN() const { return fqn; }
    size_t ChildCount() const;
    WeakType GetChild(const string &name) const;
    NameTypePair GetChild(size_t index) const;

    bool IsProductType() const { return tag == PRODUCT_TYPE; }
    bool IsSumType() const { return tag == SUM_TYPE; }
    bool IsTypeFun() const { return tag == TYPE_FUN; }
    bool IsRefType() const { return tag == REF_TYPE; }

    const auto &AsTypeFun() const { return std::get<TypeFun>(type_fun); }
    const auto &AsSumType() const { return std::get<SumType>(sum_type); }
    const auto &AsProductType() const { return std::get<ProductType>(product_type); }
    const auto &AsRefType() const { return std::get<RefType>(ref_type); }

private:
    /**
     * The tag of this type to identify the payload union.
     */
    Tag tag;

    /**
     * FQN of this type
     */
    const string fqn;

    std::variant<ProductType, SumType, TypeFun, RefType> product_type, sum_type, type_fun, ref_type;

    // Limit no name types for now
    Type(const ProductType &prod_type);
    Type(const SumType &sum_type);
    Type(const TypeFun &typefun);
    Type(const RefType &reftype);
};

END_NS

#endif

