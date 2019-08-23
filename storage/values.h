
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
    virtual int Compare(const Value &another) const = 0;
    virtual bool Equals(const Value &another) const;
    virtual bool operator< (const Value& another) const;

    /**
     * Values can be containers.
     */
    virtual bool HasChildren() const;
    virtual size_t ChildCount() const;
    virtual bool IsKeyed() const;
    virtual vector<string> Keys() const;
    virtual bool IsIndexed() const;
    virtual Value *Get(size_t index) const;
    virtual Value *Get(const std::string &key) const;
    // Setters
    virtual Value *Set(size_t index, Value *newvalue);
    virtual Value *Set(const std::string &key, Value *newvalue);
};

/**
 * Writes value to an output stream as JSON.
 */
void ValueToJson(const Value *value,
                 ostream &out, 
                 bool newlines = true,
                 int indent = 2,
                 int level = 0);

template <typename T>
class LeafValue : public Value {
public:
    LeafValue(const T &val) : value(val) { }
    int Compare(const Value &another) const {
        const LeafValue<T> *ourtype = dynamic_cast<const LeafValue<T> *>(&another);
        if (!ourtype) {
            return this - ourtype;
        }
        return Comparer<T>()(value, ourtype->value);
    }
    size_t HashCode() const { return hasher(value); }
    const T &Value() const { return value; }

protected:
    T value;
    static std::hash<T> hasher;
};

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

extern int CompareValueVector(const ValueVector &first, const ValueVector &second);
extern int CompareValueList(const ValueList &first, const ValueList &second);
extern int CompareValueMap(const ValueMap &first, const ValueMap &second);

/// Helpers to box and unbox values of literal types

template <typename T>
struct Boxer {
    Value *operator()(const T &value) const {
        return new LeafValue(value);
    }
};

template <typename T>
struct Unboxer {
    bool operator()(const Value *input, T &output) const {
        const LeafValue<T> *ourtype = dynamic_cast<const LeafValue<T> *>(input);
        if (!ourtype) {
            return false;
        }
        output = ourtype->Value();
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

END_NS

#endif

