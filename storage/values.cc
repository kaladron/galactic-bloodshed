
#include "storage/storage.h"

START_NS

//////////////////  Value Implementation  //////////////////

bool Value::Equals(const Value &another) const {
    return Compare(another) == 0;
}

bool Value::operator< (const Value& another) const {
    return Compare(another) < 0;
}

size_t Value::ChildCount() const { return 0; }
bool Value::IsKeyed() const { return false; }
vector<string> Value::Keys() const { return {}; }
bool Value::IsIndexed() const { return false; }
Value *Value::Get(size_t index) const { return nullptr; }
Value *Value::Get(const std::string &key) const { return nullptr; }

Value *Value::Set(size_t index, Value *newvalue) { }
Value *Value::Set(const std::string &key, Value *newvalue) { }




/////////////////  MapValue Implementation  /////////////////

size_t MapValue::HashCode() const {
    int h = 0;
    for (auto it : values) {
        h += std::hash<string>{}(it.first);
        h += (it.second)->HashCode();
    }
    return h;
}

Value *MapValue::Get(const string &key) const {
    auto it = values.find(key);
    if (it == values.end()) return nullptr;
    return it->second;
}

Value *MapValue::Set(const std::string &key, Value *newvalue) {
    Value *oldvalue = nullptr;
    auto it = values.find(key);
    if (it != values.end()) {
        oldvalue = it->second;
    }
    values[key] = newvalue;
    return oldvalue;
}

int MapValue::Compare(const Value &another) const {
    const MapValue *that = dynamic_cast<const MapValue *>(&another);
    if (!that) {
        return this < that;
    }
    return CompareValueMap(values, that->values);
}

size_t ListValue::HashCode() const {
    size_t hash = 0;
    for (auto it : values) {
        hash += it->HashCode();
    }
    return hash;
}

int ListValue::Compare(const Value &another) const {
    const ListValue *that = dynamic_cast<const ListValue *>(&another);
    if (!that) {
        return this < that;
    }
    return CompareValueVector(values, that->values);
}

Value *ListValue::Set(size_t index, Value *newvalue) {
    Value *oldvalue = values[index];
    values[index] = newvalue;
    return oldvalue;
}

int CompareValueVector(const ValueVector &first, const ValueVector &second) {
    return IterCompare(
            first.begin(), first.end(),
            second.begin(), second.end(),
            [](const Value *a, const Value *b) {
                return a->Compare(*b);
            });
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

END_NS
