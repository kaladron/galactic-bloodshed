
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

void Collection::Put(Entity *entity) {
    if (entity) Put(*entity);
}

void Collection::Delete(const Value *key) {
    Delete(*key);
}

void Collection::Delete(const Entity *entity) {
    Delete(entity->GetKey());
}

END_NS
