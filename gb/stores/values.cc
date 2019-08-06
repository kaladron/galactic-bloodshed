
#include "values.h"

bool MapValue::operator< (const Value& another) const {
    const MapValue *ourtype = dynamic_cast<const MapValue *>(&another);
    if (ourtype) {
        return values < ourtype->values;
    }
    return this < ourtype;
}
