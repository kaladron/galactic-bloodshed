
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

Collection::~Collection() {
}

bool Collection::Delete(const Value &value) {
    return DeleteByKey(*schema->GetKey(value));
}

bool Collection::Delete(const std::list<const Value *> &keys) {
    for (auto key : keys) {
        Delete(*key);
    }
    return true;
}

bool Collection::Put(Value *entity) {
    if (!entity) return false;
    return Put(*entity);
}

bool Collection::Put(const std::list<Value *> &values) {
    for (auto value : values) {
        Put(value);
    }
    return true;
}

END_NS
