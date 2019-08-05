
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// #include "gb/utils/macros_to_be_replaced_by_boost.h"
#include "gb/stores/sqlstore.h"
#include "gb/stores/memstore.h"
#include "gb/stores/fields.h"
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

size_t EntityClass::FieldCount() const { return field_list.size(); }
Field *EntityClass::GetField(size_t index) const { return field_list[index]; }
Field *EntityClass::GetField(const string & name) {
    return field_map[name];
}

void EntityClass::AddField(Field *field) {
    field_list.push_back(field);
    field_map[field->Name()] = field;
}

/**
 * EntityBase
 */

size_t EntityBase::FieldCount() const {
    return fields_by_name.size();
}

Field *EntityBase::GetField(const string & name) {
    return fields_by_name[name];
}

void EntityBase::SetField(const string &name, Field *field) {
    fields_by_name[name] = field;
}

EntityBase::~EntityBase() {
};

#define SET_FIELD(field_name, field_type)           \
    SetField(#field_name, (Field *)field_name);

#define SET_FIELD_MACRO(r, data, elem)          \
    SET_FIELD(BOOST_PP_TUPLE_ELEM(0, elem), BOOST_PP_TUPLE_ELEM(1, elem))

#define DECLARE_FIELD(field_name, field_type)           \
    const field_type* field_name = new field_type(#field_name);

#define DECLARE_FIELD_MACRO(r, data, elem)          \
    DECLARE_FIELD(BOOST_PP_TUPLE_ELEM(0, elem), BOOST_PP_TUPLE_ELEM(1, elem))

#define DECLARE_ENTITY(cls, fqn, fields)                        \
    class cls : public EntityBase {                             \
        cls() {                                                 \
            BOOST_PP_SEQ_FOR_EACH(SET_FIELD_MACRO, _, fields)   \
        }                                                       \
    public:                                                     \
       BOOST_PP_SEQ_FOR_EACH(DECLARE_FIELD_MACRO, _, fields)    \
    };

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
