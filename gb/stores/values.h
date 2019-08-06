
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef VALUES_H
#define VALUES_H

#include <assert.h>
#include <cstdlib>
#include <memory>
#include <utility>
#include <tuple>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <type_traits>
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include "gb/stores/types.h"

using namespace std;

class Value {
public:
    Value(Type *t) : type(t), exists(false) { }
    virtual bool matchesType(Type *type) { return false; }
    bool Exists() const { return exists; }
    void Erase() { exists = false; }
    virtual bool operator< (const Value& another) const {
        return this < &another;
    }

protected:
    Type *type;
    bool exists;
};

class MapValue : public Value {
public:
    MapValue(Type *t) : Value(t) { }
    virtual bool operator< (const Value& another) const;

protected:
    map<string, Value *> values;
};

class ListValue : public Value {
public:
    ListValue(Type *t) : Value(t) { }
    virtual bool operator< (const Value& another) const;

protected:
    list<Value *> values;
};

template <typename T>
class LeafValue : public Value {
public:
    virtual bool operator< (const Value& another) const {
        LeafValue *ourtype = dynamic_cast<LeafValue<T> *>(another);
        if (ourtype) {
            return value < ourtype->value;
        }
        return this < ourtype;
    }

protected:
    T value;
};

class TupleValue : public Value {
protected:
    list<Value *> values;
};

#endif

