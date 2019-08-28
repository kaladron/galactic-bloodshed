
// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef VALUES_H
#define VALUES_H

#include "storage/types.h"

START_NS

using ValueMap = std::map<std::string, Value *>;
using ValueList = std::list<Value *>;
using ValueVector = std::vector<Value *>;

enum class LiteralType {
    None,
    Bool,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Int8,
    Int16,
    Int32,
    Int64,
    Float,
    Double,
    String,
    Bytes
};

/**
 * Value is a super-interface for holding a hierarchy of typed values.
 */
class Value {
public:
    Value() { }
    virtual size_t HashCode() const = 0;
    virtual int Compare(const Value &another) const = 0;
    virtual bool Equals(const Value &another) const;
    virtual bool operator< (const Value& another) const;

    /**
     * Values can be containers or literals (but not both).
     */
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const;

    // For values with children indexed via ints
    virtual bool IsIndexed() const;
    virtual Value *Get(size_t index) const;
    virtual Value *Set(size_t index, Value *newvalue);

    // For values with children indexed via string keys
    virtual bool IsKeyed() const;
    virtual vector<string> Keys() const;
    virtual Value *Get(const std::string &key) const;
    virtual Value *Set(const std::string &key, Value *newvalue);
};

class Literal : public Value {
public:
    virtual LiteralType LitType() const { return LiteralType::None; }
    // Writes the value out to an output stream, possibly as a string if required
    virtual string AsString() const = 0;
    static const Literal *From(const Value *v);
    static Literal *From(Value *v);
};

template <typename T>
class TypedLiteral : public Literal {
public:
    TypedLiteral(const T &val) : value(val) { }
    int Compare(const Value &another) const {
        const TypedLiteral<T> *ourtype = dynamic_cast<const TypedLiteral<T> *>(&another);
        if (!ourtype) {
            return this - ourtype;
        }
        return Comparer<T>()(value, ourtype->value);
    }
    size_t HashCode() const { 
        std::hash<T> hasher;
        return hasher(value); 
    }
    const T &LitVal() const { return value; }
    LiteralType LitType() const { return LEAF_TYPE; }
    string AsString() const { return to_string(value); }

protected:
    T value;
    const static LiteralType LEAF_TYPE;
};

// Strings dont need a conversion!
template <> string TypedLiteral<string>::AsString() const;

class MapValue : public Value {
public:
    MapValue() { }
    virtual int Compare(const Value &another) const;
    virtual size_t HashCode() const;
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual Value *Get(const string &key) const;
    virtual Value *Set(const std::string &key, Value *newvalue);
    virtual vector<string> Keys() const;
    virtual bool IsKeyed() const { return true; }

protected:
    mutable ValueMap values;
};

class ListValue : public Value {
public:
    ListValue() { }
    virtual int Compare(const Value &another) const;
    virtual size_t HashCode() const;
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual Value *Get(size_t index) const { return values[index]; }
    virtual Value *Set(size_t index, Value *newvalue);
    virtual bool IsIndexed() const { return true; }

protected:
    std::vector<Value *> values;
};

/// Helpers to box and unbox values of literal types

template <typename T>
struct Boxer {
    Value *operator()(const T &value) const {
        return new TypedLiteral(value);
    }
};

template <typename T>
struct Unboxer {
    bool operator()(const Value *input, T &output) const {
        const TypedLiteral<T> *ourtype = dynamic_cast<const TypedLiteral<T> *>(input);
        if (!ourtype) {
            return false;
        }
        output = ourtype->LitVal();
        return true;
    }
};

const auto CharBoxer = Boxer<char>();
const auto IntBoxer = Boxer<int>();
const auto UIntBoxer = Boxer<unsigned>();
const auto LongBoxer = Boxer<long>();
const auto ULongBoxer = Boxer<unsigned long>();
const auto StringBoxer = Boxer<string>();

const auto CharUnboxer = Unboxer<char>();
const auto IntUnboxer = Unboxer<int>();
const auto UIntUnboxer = Unboxer<unsigned>();
const auto LongUnboxer = Unboxer<long>();
const auto ULongUnboxer = Unboxer<unsigned long>();
const auto StringUnboxer = Unboxer<string>();

// Helper methods

extern int CompareValueVector(const ValueVector &first, const ValueVector &second);
extern int CompareValueList(const ValueList &first, const ValueList &second);
extern int CompareValueMap(const ValueMap &first, const ValueMap &second);
void DFSWalkValue(const Value *root, FieldPath &fp, 
                  std::function<bool(int, const string *,
                                     const Value*, FieldPath &)> callback);

/**
 * Writes value to an output stream as JSON.
 */
extern void ValueToJson(const Value *value,
                        ostream &out, 
                        bool newlines = true, 
                        int indent = 2, 
                        int level = 0);

END_NS

#endif

