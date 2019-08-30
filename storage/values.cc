
#include "storage/storage.h"

START_NS

template<> const LiteralType TypedLiteral<bool>::LEAF_TYPE = LiteralType::Bool;
template<> const LiteralType TypedLiteral<uint8_t>::LEAF_TYPE = LiteralType::UInt8;
template<> const LiteralType TypedLiteral<uint16_t>::LEAF_TYPE = LiteralType::UInt16;
template<> const LiteralType TypedLiteral<uint32_t>::LEAF_TYPE = LiteralType::UInt32;
template<> const LiteralType TypedLiteral<uint64_t>::LEAF_TYPE = LiteralType::UInt64;
template<> const LiteralType TypedLiteral<int8_t>::LEAF_TYPE = LiteralType::Int8;
template<> const LiteralType TypedLiteral<int16_t>::LEAF_TYPE = LiteralType::Int16;
template<> const LiteralType TypedLiteral<int32_t>::LEAF_TYPE = LiteralType::Int32;
template<> const LiteralType TypedLiteral<int64_t>::LEAF_TYPE = LiteralType::Int64;
template<> const LiteralType TypedLiteral<float>::LEAF_TYPE = LiteralType::Float;
template<> const LiteralType TypedLiteral<double>::LEAF_TYPE = LiteralType::Double;
template<> const LiteralType TypedLiteral<string>::LEAF_TYPE = LiteralType::String;
template<> const LiteralType TypedLiteral<stringbuf>::LEAF_TYPE = LiteralType::Bytes;

//////////////////  Value Implementation  //////////////////

bool Value::Equals(const Value *another) const {
    return Compare(another) == 0;
}

bool Value::operator< (const Value *another) const {
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

template <>
string TypedLiteral<string>::AsString() const
{
    return value;
}

const Literal *Literal::From(const Value *v) {
    return dynamic_cast<const Literal *>(v);
}

Literal *Literal::From(Value *v) {
    return dynamic_cast<Literal *>(v);
}

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
        const Literal *lit = Literal::From(value);
        assert(lit != nullptr && "Non literal value found");
        if (lit->LitType() == LiteralType::String) {
            out << '"' << lit->AsString() << '"';
        } else {
            lit->AsString();
        }
    }
}

/////////////////  MapValue Implementation  /////////////////

UnionValue::UnionValue(int t, Value *d) : tag(t), data(d) {
}

size_t UnionValue::HashCode() const {
    return tag + data->HashCode();
}

int UnionValue::Compare(const Value *another) const {
    const UnionValue *uv = dynamic_cast<const UnionValue *>(another);
    if (!uv) return (const Value *)this - another;
    if (uv->tag != tag) return tag - uv->tag;
    return data->Compare(uv->data);
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

int MapValue::Compare(const Value *another) const {
    const MapValue *that = dynamic_cast<const MapValue *>(another);
    if (!that) {
        return (const Value *)this - another;
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

int ListValue::Compare(const Value *another) const {
    const ListValue *that = dynamic_cast<const ListValue *>(another);
    if (!that) {
        return (const Value *)this - another;
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
                return a->Compare(b);
            });
}

int CompareValueList(const ValueList &first, const ValueList &second) {
    return IterCompare(
            first.begin(), first.end(),
            second.begin(), second.end(),
            [](const Value *a, const Value *b) {
                return a->Compare(b);
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
                cmp = a.second->Compare(b.second);
                if (cmp != 0) return cmp;
            });
}

// bool (*callback)(int index, const string *key, const Value *value, FieldPath &fp)) {
void DFSWalkValue(const Value *root, FieldPath &fp, 
                  std::function<bool(int, const string *,
                                     const Value*, FieldPath &)> callback) {
    if (!root) {
        return ;
    } else if (root->IsKeyed()) {
        int i = 0;
        for (auto key : root->Keys()) {
            auto child = root->Get(key);
            fp.push_back(key);
            if (callback(i++, &key, child, fp)) {
                DFSWalkValue(child, fp, callback);
            }
            fp.pop_back();
        }
    } else if (root->IsIndexed()) {
        for (int i = 0, s = root->ChildCount();i < s;i++) {
            auto child = root->Get(i);
            fp.push_back(to_string(i));
            if (callback(i, nullptr, child, fp)) {
                DFSWalkValue(child, fp, callback);
            }
            fp.pop_back();
        }
    } else {
    }
}

bool MatchTypeAndValue(const Type *type, const Value *root, 
                       int currIndex, FieldPath &fp, 
                       std::function<bool(const Type *, const Value*,
                                          int, const string *, FieldPath &)> callback) {
    if (!type && !root) return true;
    if (!type || !root) return false;

    // Call with the current root and type first before descending
    if (!callback(type, root, currIndex, fp.empty() ? nullptr : &(fp.back()), fp)) return true;

    if (type->IsRecord()) {
        if (!root->IsKeyed()) return false;
        // descend into children
        for (int i = 0, c = type->ChildCount();i < c;i++) {
            const auto &childtype = type->GetChild(i);
            auto &key = childtype.first;
            auto childvalue = root->Get(key);
            fp.push_back(key);
            if (!MatchTypeAndValue(childtype.second,
                                   childvalue,
                                   i, fp,
                                   callback)) return false;
            fp.pop_back();
        }
    }
    else if (type->IsUnion()) {
        const UnionValue *uv = dynamic_cast<const UnionValue *>(root);
        assert(uv != nullptr && "Expected Union Value");
        if (uv->Tag() <= type->ChildCount()) {
            // Ensure value's tag is not in a forward version than type so ignore
            const auto childtype = type->GetChild(uv->Tag());
            const auto childvalue = uv->Data();
            auto &key = childtype.first;
            fp.push_back(key);
            if (!MatchTypeAndValue(childtype.second,
                                   childvalue,
                                   uv->Tag(), fp,
                                   callback)) return false;
            fp.pop_back();
        }
    } else {    // type function
        // TODO: make this pluggable instead of hard coded
        if (type->FQN() == "list") {
            if (!root->IsIndexed()) {
                return false;
            }
            const auto &childtype = type->GetChild(0);
            for (int i = 0, c = root->ChildCount();i < c;i++) {
                auto childvalue = root->Get(i);
                fp.push_back(to_string(i));
                if (!MatchTypeAndValue(childtype.second,
                                       childvalue,
                                       i, fp,
                                       callback)) return false;
                fp.pop_back();
            }
        } else if (type->FQN() == "map") {
            if (!root->IsKeyed()) {
                return false;
            }
            const auto &childtype = type->GetChild(0);
            int i = 0;
            for (auto key : root->Keys()) {
                auto childvalue = root->Get(key);
                fp.push_back(key);
                if (!MatchTypeAndValue(childtype.second,
                                       childvalue,
                                       i++, fp,
                                       callback)) return false;
                fp.pop_back();
            }
        } else {
            // literal values
            assert(false && "TBD");
        }
    /*
    if (root->IsKeyed()) {
    } else if (root->IsIndexed()) {
        for (int i = 0, s = root->ChildCount();i < s;i++) {
            auto child = root->Get(i);
            fp.push_back(to_string(i));
            if (callback(i, nullptr, child, fp)) {
                DFSWalkValue(child, fp, callback);
            }
            fp.pop_back();
        }
    } else {
    }
    */
    }
}

bool MatchTypeAndValue(const Type *type, const Value *value, 
                       std::function<bool(const Type *, const Value*,
                                          int, const string *, FieldPath &)> callback) {
    FieldPath fp;
    MatchTypeAndValue(type, value, 0, fp, callback);
}

END_NS
