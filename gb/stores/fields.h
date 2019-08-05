
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FIELDS_H
#define FIELDS_H

#include "entities.h"

using namespace std;

template<typename EntityClass>
class StructField : public Field {
private:
    unique_ptr<Field> child_field;
};

class ListField : public Field {
public:
    ListField(const string &name, unique_ptr<Field> child) 
        : Field(name), child_field(move(child)) { }

private:
    unique_ptr<Field> child_field;
};

template<typename ValueType>
class LeafField : public Field {
public:
    LeafField(const string &name) : Field(name) { }
};

template <typename ... Ks>
class KeyField : public LeafField<Key<Ks...>> {
};

#endif
