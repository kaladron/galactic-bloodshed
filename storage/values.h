
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef VALUES_H
#define VALUES_H

#include "storage/types.h"

START_NS

using ValueMap = std::map<std::string, Value *>;
using ValueList = std::list<Value *>;
using ValueVector = std::vector<Value *>;

class Value {
public:
    Value(const Type *t) : type(t), exists(false) { }
    virtual bool matchesType(const Type *type) { return false; }
    bool Exists() const { return exists; }
    void Erase() { exists = false; }
    virtual Value *ResolveFieldPath(FieldPath *path) { return nullptr; }
    virtual size_t HashCode() const { }
    virtual int Compare(const Value &another) const { }
    virtual bool Equals(const Value &another) const { return Compare(another) == 0; }
    virtual bool operator< (const Value& another) const { return Compare(another) < 0; }

protected:
    const Type *type;
    bool exists;
};

class MapValue : public Value {
public:
    MapValue(const Type *t) : Value(t) { }
    virtual int Compare(const Value &another) const;
    virtual size_t HashCode() const;
    virtual size_t Size() const { return values.size(); }
    virtual Value *ValueForKey(const std::string &key) const;

protected:
    mutable ValueMap values;
};

class ListValue : public Value {
public:
    ListValue(const Type *t) : Value(t) { }
    virtual int Compare(const Value &another) const;
    virtual size_t HashCode() const;
    virtual size_t Size() const { return values.size(); }
    virtual Value *At(size_t index) const { return values[index]; }

protected:
    std::vector<Value *> values;
};

template <typename T>
class LeafValue : public Value {
public:
    virtual int Compare(const T &another) const {
        return value - another;
    }
    virtual int Compare(const Value &another) const {
        LeafValue *ourtype = dynamic_cast<LeafValue<T> *>(another);
        if (!ourtype) {
            return this - ourtype;
        }
        return Compare<T>(value, ourtype->value);
    }
    virtual size_t HashCode() const { return hasher(value); }

protected:
    T value;
    static std::hash<T> hasher;
};

extern int CompareValueVector(const ValueVector &first, const ValueVector &second);
extern int CompareValueList(const ValueList &first, const ValueList &second);
extern int CompareValueMap(const ValueMap &first, const ValueMap &second);

END_NS

#endif

