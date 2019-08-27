
#include "storage/storage.h"

START_NS

template<> const LeafType LeafValue<bool>::LEAF_TYPE = LeafType::BOOL;
template<> const LeafType LeafValue<uint8_t>::LEAF_TYPE = LeafType::UINT8;
template<> const LeafType LeafValue<uint16_t>::LEAF_TYPE = LeafType::UINT16;
template<> const LeafType LeafValue<uint32_t>::LEAF_TYPE = LeafType::UINT32;
template<> const LeafType LeafValue<uint64_t>::LEAF_TYPE = LeafType::UINT64;
template<> const LeafType LeafValue<int8_t>::LEAF_TYPE = LeafType::INT8;
template<> const LeafType LeafValue<int16_t>::LEAF_TYPE = LeafType::INT16;
template<> const LeafType LeafValue<int32_t>::LEAF_TYPE = LeafType::INT32;
template<> const LeafType LeafValue<int64_t>::LEAF_TYPE = LeafType::INT64;
template<> const LeafType LeafValue<float>::LEAF_TYPE = LeafType::FLOAT;
template<> const LeafType LeafValue<double>::LEAF_TYPE = LeafType::DOUBLE;
template<> const LeafType LeafValue<string>::LEAF_TYPE = LeafType::STRING;
template<> const LeafType LeafValue<stringbuf>::LEAF_TYPE = LeafType::BYTES;

//////////////////  Value Implementation  //////////////////

bool Value::Equals(const Value &another) const {
    return Compare(another) == 0;
}

bool Value::operator< (const Value& another) const {
    return Compare(another) < 0;
}

bool Value::HasChildren() const { return ChildCount() == 0; }
size_t Value::ChildCount() const { return 0; }
bool Value::IsKeyed() const { return false; }
vector<string> Value::Keys() const { return {}; }
bool Value::IsIndexed() const { return false; }
Value *Value::Get(size_t index) const { return nullptr; }
Value *Value::Get(const std::string &key) const { return nullptr; }

Value *Value::Set(size_t index, Value *newvalue) { }
Value *Value::Set(const std::string &key, Value *newvalue) { }

void ValueToJson(const Value *value, ostream &out, 
                 bool newlines, int indent, int level) {
    if (!value) {
        out << "null";
    } else if (value->IsKeyed()) {
        out << "{";
        int i = 0;
        for (auto key : value->Keys()) {
            if (i++ > 0) out << ", ";
            out << '"' << key << '"' << ": ";
            auto child = value->Get(key);
            ValueToJson(child, out, newlines, indent, level + 1);
        }
        out << "}";
    } else if (value->IsIndexed()) {
        out << "[";
        for (int i = 0, s = value->ChildCount();i < s;i++) {
            if (i > 0) out << ", ";
            auto child = value->Get(i);
            ValueToJson(child, out, newlines, indent, level + 1);
        }
        out << "]";
    } else {
        const LeafValue<string> *leaf = dynamic_cast<const LeafValue<string> *>(value);
        if (leaf) {
            out << '"';
            value->Write(out);
            out << '"';
        } else {
            value->Write(out);
        }
    }
}

/////////////////  MapValue Implementation  /////////////////

size_t MapValue::HashCode() const {
    int h = 0;
    for (auto it : values) {
        h += std::hash<string>{}(it.first);
        h += (it.second)->HashCode();
    }
    return h;
}

bool MapValue::HasChildren() const {
    return !values.empty();
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

vector<string> MapValue::Keys() const {
    vector<string> out;
    for (auto it : values) {
        out.push_back(it.first);
    }
    return out;
}

/////////////////  ListValue Implementation  /////////////////

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

bool ListValue::HasChildren() const {
    return !values.empty();
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
