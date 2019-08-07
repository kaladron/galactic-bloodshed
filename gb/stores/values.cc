
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
    return IterCompare(
            first.begin(), first.end(),
            second.begin(), second.end(),
            [](const Value *a, const Value *b) {
                return a->Compare(*b);
            });
}

int CompareValueMap(const ValueMap &first, const ValueMap &second) {
    return IterCompare(
            first.begin(), first.end(),
            second.begin(), second.end(),
            [](const pair<string, Value *> &a, const pair<string, Value *> &b) {
                int cmp = a.first.compare(b.first);
                if (cmp != 0) return cmp;

                // check value otherwise
                cmp = a.second->Compare(*(b.second));
                if (cmp != 0) return cmp;
            });
}
