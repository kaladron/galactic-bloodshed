
// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef VALUES_H
#define VALUES_H

#include <iostream>
#include "storage/types.h"

START_NS

using ValueMap = std::map<std::string, StrongValue>;
using ValueList = std::list<StrongValue>;
using ValueVector = std::vector<StrongValue>;

/**
 * Value is a super-interface for holding a hierarchy of typed values.
 */
class Value {
public:
    Value() { }
    virtual ~Value() { }
    // virtual Value *Copy(const Value &another) = 0;
    virtual size_t HashCode() const = 0;
    virtual ostream &Write(ostream &) const;
    virtual int Compare(const Value *another) const = 0;
    virtual bool Equals(const Value *another) const;
    virtual int Compare(StrongValue another) const;
    virtual bool Equals(StrongValue another) const;
    // virtual int Compare(const Value &another) const { return Compare(&another); }
    // virtual bool Equals(const Value &another) const { return Equals(&another); }
    virtual bool operator< (const Value* another) const;
    friend ostream & operator << (ostream &out, const Value &v);

    /**
     * Values can be containers or literals (but not both).
     */
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const;

    // For values with children indexed via ints
    virtual bool IsIndexed() const;
    virtual StrongValue Get(size_t index) const;
    virtual StrongValue Set(size_t index, StrongValue newvalue);

    // For values with children indexed via string keys
    virtual bool IsKeyed() const;
    // TODO: Need a better key or key/val iterator
    virtual vector<string> Keys() const;
    virtual StrongValue Get(const std::string &key) const;
    virtual StrongValue Set(const std::string &key, StrongValue newvalue);
};

class MapValue : public Value {
public:
    MapValue();
    MapValue(std::initializer_list<std::pair<const string, StrongValue>> vals);
    MapValue(const ValueMap &vals);
    virtual int Compare(const Value *another) const;
    virtual size_t HashCode() const;
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual StrongValue Get(const string &key) const;
    virtual StrongValue Set(const std::string &key, StrongValue newvalue);
    virtual vector<string> Keys() const;
    virtual bool IsKeyed() const { return true; }

protected:
    mutable ValueMap values;
};

class ListValue : public Value {
public:
    ListValue();
    ListValue(std::initializer_list<StrongValue> values);
    ListValue(ValueVector &vals);
    virtual int Compare(const Value *another) const;
    virtual size_t HashCode() const;
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual StrongValue Get(size_t index) const;
    virtual StrongValue Set(size_t index, StrongValue newvalue);
    virtual bool IsIndexed() const { return true; }

protected:
    ValueVector values;
};

class UnionValue : public Value {
public:
    UnionValue(int t, StrongValue d);
    virtual size_t HashCode() const;
    virtual int Compare(const Value *another) const;
    int Tag() const { return tag; }
    StrongValue Data() const { return data; }

private:
    int tag;
    StrongValue data;
};

/// Helpers to box and unbox values of literal types

enum LiteralType {
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

class Literal : public Value {
public:
    virtual LiteralType LitType() const { return LiteralType::None; }
    // Writes the value out to an output stream, possibly as a string if required
    virtual string AsString() const = 0;
    static const Literal *From(const Value *v);
    static Literal *From(Value *v);
    virtual int Compare(const Value *another) const;
};

template <typename T>
class TypedLiteral : public Literal {
public:
    TypedLiteral(const T &val) : value(val) { }
    int Compare(const Value *another) const {
        const TypedLiteral<T> *ourtype = dynamic_cast<const TypedLiteral<T> *>(another);
        if (ourtype == nullptr) {
            // see if it is atleast a literal
            return Literal::Compare(another);
        }
        return Comparer<T>()(value, ourtype->value);
    }
    size_t HashCode() const { 
        std::hash<T> hasher;
        return hasher(value); 
    }
    const T &LitVal() const { return value; }
    LiteralType LitType() const { return LIT_TYPE; }
    string AsString() const { return std::to_string(value); }
    ostream &Write(ostream &out) const { out << value; return out; }

protected:
    T value;
    const static LiteralType LIT_TYPE;
};

// Bools need custom conv
template <> string TypedLiteral<bool>::AsString() const;
// Strings dont need a conversion!
template <> string TypedLiteral<string>::AsString() const;

template <typename T>
struct Boxer {
    shared_ptr<Literal> operator()(const T &value) const {
        return std::make_shared<TypedLiteral<T>>(value);
    }
};

template <typename T>
struct Unboxer {
    bool operator()(StrongValue input, T &output) const {
        const TypedLiteral<T> *ourtype = dynamic_cast<const TypedLiteral<T> *>(input.get());
        if (!ourtype) {
            return false;
        }
        output = ourtype->LitVal();
        return true;
    }

    T operator()(StrongValue input) const {
        T output;
        if (!(*this)(input, output)) {
            throw std::invalid_argument("Could not unbox input");
        }
        return output;
    }
};

extern const Boxer<bool> BoolBoxer;
extern const Boxer<int8_t> Int8Boxer;
extern const Boxer<int16_t> Int16Boxer;
extern const Boxer<int32_t> Int32Boxer;
extern const Boxer<int64_t> Int64Boxer;
extern const Boxer<uint8_t> UInt8Boxer;
extern const Boxer<uint16_t> UInt16Boxer;
extern const Boxer<uint32_t> UInt32Boxer;
extern const Boxer<uint64_t> UInt64Boxer;
extern const Boxer<float> FloatBoxer;
extern const Boxer<double> DoubleBoxer;
extern const Boxer<string> StringBoxer;

extern const Unboxer<bool> BoolUnboxer;
extern const Unboxer<int8_t> Int8Unboxer;
extern const Unboxer<int16_t> Int16Unboxer;
extern const Unboxer<int32_t> Int32Unboxer;
extern const Unboxer<int64_t> Int64Unboxer;
extern const Unboxer<uint8_t> UInt8Unboxer;
extern const Unboxer<uint16_t> UInt16Unboxer;
extern const Unboxer<uint32_t> UInt32Unboxer;
extern const Unboxer<uint64_t> UInt64Unboxer;
extern const Unboxer<float> FloatUnboxer;
extern const Unboxer<double> DoubleUnboxer;
extern const Unboxer<string> StringUnboxer;

// Helper methods

extern int StringValuePairCompare(const pair<string, StrongValue> &a, const pair<string, StrongValue> &b);
extern int CompareValueVector(const ValueVector &first, const ValueVector &second);
extern int CompareValueList(const ValueList &first, const ValueList &second);
extern int CompareValueMap(const ValueMap &first, const ValueMap &second);

using DFSWalkCallback = std::function<bool(const Value*, int, const string *, FieldPath &)>;
void DFSWalkValue(const Value *root, DFSWalkCallback callback);

using MatchTypeAndValueCallback = std::function<bool(const Type *, const Value*, int, const string *, FieldPath &)>;
void MatchTypeAndValue(const Type *type, const Value *value, MatchTypeAndValueCallback callback);

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

