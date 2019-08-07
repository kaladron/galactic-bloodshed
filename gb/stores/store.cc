
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// #include "gb/stores/sqlstore.h"
#include "gb/stores/memstore.h"
#include "gb/stores/sqlstore.h"

/*
DECLARE_ENTITY(AddressEntity, "Address", 
        (( street_number, LeafField<int> ))
        (( street_name, LeafField<string> ))
        (( city, LeafField<string> ))
        (( country, LeafField<string> ))
        (( zipcode, LeafField<string> ))
        (( created_timestamp, LeafField<long> ))
        (( somepair_first, LeafField<int> ))
        (( somepair_second, LeafField<float> ))
)

const Type* AddressType = new Type("Address",
        new NameTypeVector() {
            pair("street_number", IntType),
            pair("street_name", StringType),
        }, true);
*/
