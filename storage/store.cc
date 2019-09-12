
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

Collection::~Collection() {
}

bool Collection::Delete(const std::list<StrongValue> &keys) {
    for (auto key : keys) {
        Delete(key);
    }
    return true;
}

bool Collection::Put(const std::list<StrongValue> &values) {
    for (auto value : values) {
        Put(value);
    }
    return true;
}

END_NS
