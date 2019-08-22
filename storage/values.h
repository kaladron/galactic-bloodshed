
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
    virtual bool Equals(const Value &another) const { return Compare(another) == 0; }
    virtual bool operator< (const Value& another) const { return Compare(another) < 0; }

    /**
     * Values can be containers.
     */
    virtual size_t ChildCount() const { return 0; }
    virtual bool IsKeyed() const { return false; }
    virtual string GetKey(size_t index) const { return ""; }
    virtual bool IsIndexed() const { return false; }
    virtual Value *GetChild(size_t index) const { return nullptr; }
    virtual Value *GetChild(const std::string &key) const { return nullptr; }
    // Setters
    virtual Value *SetChild(size_t index, Value *newvalue) { }
    virtual Value *SetChild(const std::string &key, Value *newvalue) { }
};

template <typename T>
class LeafValue : public Value {
public:
    LeafValue(const T &val) : value(val) { }
    virtual int Compare(const Value &another) const {
        const LeafValue<T> *ourtype = dynamic_cast<const LeafValue<T> *>(&another);
        if (!ourtype) {
            return this - ourtype;
        }
        return Comparer<T>()(value, ourtype->value);
    }
    virtual size_t HashCode() const { return hasher(value); }

protected:
    T value;
    static std::hash<T> hasher;
};

class MapValue : public Value {
public:
    MapValue() { }
    virtual int Compare(const Value &another) const;
    virtual size_t HashCode() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual Value *GetChild(const string &key) const;
    virtual Value *SetChild(const std::string &key, Value *newvalue);
    virtual bool IsKeyed() const { return true; }

protected:
    mutable ValueMap values;
};

class ListValue : public Value {
public:
    ListValue() { }
    virtual int Compare(const Value &another) const;
    virtual size_t HashCode() const;
    virtual size_t ChildCount() const { return values.size(); }
    virtual Value *GetChild(size_t index) const { return values[index]; }
    virtual Value *SetChild(size_t index, Value *newvalue);
    virtual bool IsIndexed() const { return true; }

protected:
    std::vector<Value *> values;
};

template <typename T>
Value *MakeValue(const T &value) { return new LeafValue(value); }

extern int CompareValueVector(const ValueVector &first, const ValueVector &second);
extern int CompareValueList(const ValueList &first, const ValueList &second);
extern int CompareValueMap(const ValueMap &first, const ValueMap &second);

END_NS

#endif

