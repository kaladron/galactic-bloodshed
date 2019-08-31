
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

/**
 * Value is a super-interface for holding a hierarchy of typed values.
 */
class Value {
public:
    Value() { }
    virtual size_t HashCode() const = 0;
    virtual int Compare(const Value *another) const = 0;
    virtual bool Equals(const Value *another) const;
    // virtual int Compare(const Value &another) const { return Compare(&another); }
    // virtual bool Equals(const Value &another) const { return Equals(&another); }
    virtual bool operator< (const Value* another) const;

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
    // TODO: Need a better key or key/val iterator
    virtual vector<string> Keys() const;
    virtual Value *Get(const std::string &key) const;
    virtual Value *Set(const std::string &key, Value *newvalue);
};

class MapValue : public Value {
public:
    MapValue() { }
    MapValue(ValueMap &vals) : values(vals) { }
    virtual int Compare(const Value *another) const;
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
    ListValue(ValueVector &vals) : values(vals) { }
    virtual int Compare(const Value *another) const;
    virtual size_t HashCode() const;
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual Value *Get(size_t index) const;
    virtual Value *Set(size_t index, Value *newvalue);
    virtual bool IsIndexed() const { return true; }

protected:
    std::vector<Value *> values;
};

class UnionValue : public Value {
public:
    UnionValue(int t, Value *d);
    virtual size_t HashCode() const;
    virtual int Compare(const Value *another) const;
    int Tag() const { return tag; }
    Value *Data() const { return data; }

private:
    int tag;
    Value *data;
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
};

template <typename T>
class TypedLiteral : public Literal {
public:
    TypedLiteral(const T &val) : value(val) { }
    int Compare(const Value *another) const {
        const TypedLiteral<T> *ourtype = dynamic_cast<const TypedLiteral<T> *>(another);
        if (!ourtype) {
            // see if it is atleast a literal
            const Literal *littype = dynamic_cast<const Literal *>(another);
            if (littype) {
                int litcmp = LitType() - littype->LitType();
                assert (litcmp != 0 && 
                        "Literal types are same but classes are different."
                        "Multiple Literal implementations found.");
                return litcmp;
            }
            return (const Value *)this - another;
        }
        return Comparer<T>()(value, ourtype->value);
    }
    size_t HashCode() const { 
        std::hash<T> hasher;
        return hasher(value); 
    }
    const T &LitVal() const { return value; }
    LiteralType LitType() const { return LIT_TYPE; }
    string AsString() const { return to_string(value); }

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
    Literal *operator()(const T &value) const {
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

extern int CompareValueVector(const ValueVector &first, const ValueVector &second);
extern int CompareValueList(const ValueList &first, const ValueList &second);
extern int CompareValueMap(const ValueMap &first, const ValueMap &second);

using DFSWalkCallback = std::function<bool(const Value*, int, const string *, FieldPath &)>;
void DFSWalkValue(const Value *root, DFSWalkCallback callback);

using MatchTypeAndValueCallback = std::function<bool(const Type *, const Value*, int, const string *, FieldPath &)>;
bool MatchTypeAndValue(const Type *type, const Value *value, MatchTypeAndValueCallback callback);

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

