
#include "values.h"

int MapValue::Compare(const Value &another) const {
    const MapValue *that = dynamic_cast<const MapValue *>(&another);
    if (!that) {
        return this < that;
    }
    // perform an element wise comparison stopping either when no more elements exist
    // or the first key we have is not equal
    return CompareValueMap(values, that->values);
}

int ListValue::Compare(const Value &another) const {
    const ListValue *that = dynamic_cast<const ListValue *>(&another);
    if (!that) {
        return this < that;
    }
    return CompareValueList(values, that->values);
}

int CompareValueList(const ValueList &first, const ValueList &second) {
    auto it1 = first.begin();
    auto it2 = second.begin();
    for (;it1 != first.end() && it2 != second.end(); it1++, it2++) {
        int cmp = (*it1)->Compare(**it2);
        if (cmp != 0) return cmp;
    }
    if (it1 == it2) return 0;
    else if (it1 == first.end()) return -1;
    return 1;
}

int CompareValueMap(const ValueMap &first, const ValueMap &second) {
    auto it1 = first.begin();
    auto it2 = second.begin();
    for (;it1 != first.end() && it2 != second.end(); it1++, it2++) {
        auto a = *it1;
        auto b = *it2;
        int cmp = a.first.compare(b.first);
        if (cmp != 0) return cmp;

        // check value otherwise
        cmp = a.second->Compare(*(b.second));
        if (cmp != 0) return cmp;
    }
    if (it1 == it2) return 0;
    else if (it1 == first.end()) return -1;
    return 1;
}
