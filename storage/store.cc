
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "storage/storage.h"

START_NS

void Collection::Delete(const Value &value) {
    DeleteByKey(*schema->GetKey(value));
}

END_NS
