
#include "storage/storage.h"
#include <iostream>

START_NS

const Boxer<bool> BoolBoxer = Boxer<bool>();
const Boxer<int8_t> Int8Boxer = Boxer<int8_t>();
const Boxer<int16_t> Int16Boxer = Boxer<int16_t>();
const Boxer<int32_t> Int32Boxer = Boxer<int32_t>();
const Boxer<int64_t> Int64Boxer = Boxer<int64_t>();
const Boxer<uint8_t> UInt8Boxer = Boxer<uint8_t>();
const Boxer<uint16_t> UInt16Boxer = Boxer<uint16_t>();
const Boxer<uint32_t> UInt32Boxer = Boxer<uint32_t>();
const Boxer<uint64_t> UInt64Boxer = Boxer<uint64_t>();
const Boxer<float> FloatBoxer = Boxer<float>();
const Boxer<double> DoubleBoxer = Boxer<double>();
const Boxer<string> StringBoxer = Boxer<string>();

const Unboxer<bool> BoolUnboxer = Unboxer<bool>();
const Unboxer<int8_t> Int8Unboxer = Unboxer<int8_t>();
const Unboxer<int16_t> Int16Unboxer = Unboxer<int16_t>();
const Unboxer<int32_t> Int32Unboxer = Unboxer<int32_t>();
const Unboxer<int64_t> Int64Unboxer = Unboxer<int64_t>();
const Unboxer<uint8_t> UInt8Unboxer = Unboxer<uint8_t>();
const Unboxer<uint16_t> UInt16Unboxer = Unboxer<uint16_t>();
const Unboxer<uint32_t> UInt32Unboxer = Unboxer<uint32_t>();
const Unboxer<uint64_t> UInt64Unboxer = Unboxer<uint64_t>();
const Unboxer<float> FloatUnboxer = Unboxer<float>();
const Unboxer<double> DoubleUnboxer = Unboxer<double>();
const Unboxer<string> StringUnboxer = Unboxer<string>();

template<> const LiteralType TypedLiteral<bool>::LIT_TYPE = LiteralType::Bool;
template<> const LiteralType TypedLiteral<uint8_t>::LIT_TYPE = LiteralType::UInt8;
template<> const LiteralType TypedLiteral<uint16_t>::LIT_TYPE = LiteralType::UInt16;
template<> const LiteralType TypedLiteral<uint32_t>::LIT_TYPE = LiteralType::UInt32;
template<> const LiteralType TypedLiteral<uint64_t>::LIT_TYPE = LiteralType::UInt64;
template<> const LiteralType TypedLiteral<int8_t>::LIT_TYPE = LiteralType::Int8;
template<> const LiteralType TypedLiteral<int16_t>::LIT_TYPE = LiteralType::Int16;
template<> const LiteralType TypedLiteral<int32_t>::LIT_TYPE = LiteralType::Int32;
template<> const LiteralType TypedLiteral<int64_t>::LIT_TYPE = LiteralType::Int64;
template<> const LiteralType TypedLiteral<float>::LIT_TYPE = LiteralType::Float;
template<> const LiteralType TypedLiteral<double>::LIT_TYPE = LiteralType::Double;
template<> const LiteralType TypedLiteral<string>::LIT_TYPE = LiteralType::String;
template<> const LiteralType TypedLiteral<stringbuf>::LIT_TYPE = LiteralType::Bytes;

//////////////////  Value Implementation  //////////////////

bool Value::Equals(const Value *another) const {
    return Compare(another) == 0;
}
bool Value::Equals(StrongValue another) const {
    return Compare(another.get()) == 0;
}
int Value::Compare(StrongValue another) const {
    return Compare(another.get());
}

bool Value::operator< (const Value *another) const {
    return Compare(another) < 0;
}

bool Value::HasChildren() const { return ChildCount() == 0; }
size_t Value::ChildCount() const { return 0; }
bool Value::IsKeyed() const { return false; }
vector<string> Value::Keys() const { return {}; }
bool Value::IsIndexed() const { return false; }
StrongValue Value::Get(size_t index) const { return StrongValue(); }
StrongValue Value::Get(const std::string &key) const { return StrongValue(); }

StrongValue Value::Set(size_t index, StrongValue newvalue) { }
StrongValue Value::Set(const std::string &key, StrongValue newvalue) { }

template <>
string TypedLiteral<bool>::AsString() const
{
    return value ? "true" : "false";
}

template <>
string TypedLiteral<string>::AsString() const
{
    return value;
}

int Literal::Compare(const Value *another) const {
    const Literal *littype = Literal::From(another);
    if (littype) {
        int litcmp = LitType() - littype->LitType();
        assert (litcmp != 0 && 
                "Literal types are same but classes are different."
                "Multiple Literal implementations found.");
        cout << "Returning lit cmp: " << LitType() << ", " << littype->LitType() << endl;
        return litcmp;
    }
    return (const Value *)this - another;
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
            auto child = value->Get(key).get();
            ValueToJson(child, out, newlines, indent, level + 1);
        }
        out << "}";
    } else if (value->IsIndexed()) {
        out << "[";
        for (int i = 0, s = value->ChildCount();i < s;i++) {
            if (i > 0) out << ", ";
            auto child = value->Get(i).get();
            ValueToJson(child, out, newlines, indent, level + 1);
        }
        out << "]";
    } else {
        const Literal *lit = Literal::From(value);
        assert(lit != nullptr && "Non literal value found");
        if (lit->LitType() == LiteralType::String) {
            out << '"' << lit->AsString() << '"';
        } else {
            out << lit->AsString();
        }
    }
}

/////////////////  MapValue Implementation  /////////////////

UnionValue::UnionValue(int t, StrongValue d) : tag(t), data(d) {
}

size_t UnionValue::HashCode() const {
    return tag + data->HashCode();
}

int UnionValue::Compare(const Value *another) const {
    const UnionValue *uv = dynamic_cast<const UnionValue *>(another);
    if (!uv) return (const Value *)this - another;
    if (uv->tag != tag) return tag - uv->tag;
    return data->Compare(uv->data.get());
}

/////////////////  MapValue Implementation  /////////////////

MapValue::MapValue(ValueMap &vals) : values(vals) { }

size_t MapValue::HashCode() const {
    std::hash<string> hasher;
    size_t h = 0;
    for (auto it : values) {
        h += hasher(it.first);
        h += (it.second)->HashCode();
    }
    return h;
}

bool MapValue::HasChildren() const {
    return !values.empty();
}

StrongValue MapValue::Get(const string &key) const {
    auto it = values.find(key);
    if (it == values.end()) return nullptr;
    return it->second;
}

StrongValue MapValue::Set(const std::string &key, StrongValue newvalue) {
    StrongValue oldvalue;
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

StrongValue ListValue::Get(size_t index) const {
    if (index >= values.size()) return StrongValue();
    return values[index];
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

StrongValue ListValue::Set(size_t index, StrongValue newvalue) {
    StrongValue oldvalue = values[index];
    values[index] = newvalue;
    return oldvalue;
}

int CompareValueVector(const ValueVector &first, const ValueVector &second) {
    return IterCompare(
            first.begin(), first.end(),
            second.begin(), second.end(),
            [](StrongValue a, StrongValue b) {
                return a->Compare(b);
            });
}

int CompareValueList(const ValueList &first, const ValueList &second) {
    return IterCompare(
            first.begin(), first.end(),
            second.begin(), second.end(),
            [](StrongValue a, StrongValue b) {
                return a->Compare(b);
            });
}

int CompareValueMap(const ValueMap &first, const ValueMap &second) {
    for (auto it : first) { cout << "First Key " << it.first << endl; }
    for (auto it : second) { cout << "Second Key " << it.first << endl; }
    auto result = IterCompare(first.begin(), first.end(), second.begin(), second.end(),
            [](const pair<string, StrongValue> &a, const pair<string, StrongValue> &b) {
                int cmp = a.first.compare(b.first);
                if (cmp != 0) return cmp;

                cmp = a.second->Compare(b.second);
                if (cmp != 0) return cmp;
            });
    cout << "Cmp Result: " << result << endl;
    return result;
}

void DFSWalkValue(const Value *root, int currIndex, FieldPath &fp, DFSWalkCallback callback) {
    if (!root) return ;

    if (!callback(root, currIndex, fp.empty() ? nullptr : &(fp.back()), fp)) return ;

    if (root->IsKeyed()) {
        int i = 0;
        for (auto key : root->Keys()) {
            auto child = root->Get(key).get();
            fp.push_back(key);
            DFSWalkValue(child, i++, fp, callback);
            fp.pop_back();
        }
    } else if (root->IsIndexed()) {
        for (int i = 0, s = root->ChildCount();i < s;i++) {
            auto child = root->Get(i).get();
            fp.push_back(to_string(i));
            DFSWalkValue(child, i, fp, callback);
            fp.pop_back();
        }
    } else {
    }
}

void DFSWalkValue(const Value *root, DFSWalkCallback callback) {
    FieldPath fp;
    DFSWalkValue(root, 0, fp, callback);
}

void MatchTypeAndValue(const Type *type, const Value *root, 
                       FieldPath &fp, MatchTypeAndValueCallback callback) {
    if (!type || !root) return ;
    // Call with the current root and type first before descending
    if (type->IsRecord()) {
        if (!root->IsKeyed()) return ;
        // descend into children
        for (int i = 0, c = type->ChildCount();i < c;i++) {
            const auto &childtype = type->GetChild(i);
            auto &key = childtype.first;
            auto childvalue = root->Get(key).get();
            fp.push_back(key);
            if (callback(childtype.second, childvalue, i, &key, fp)) {
                MatchTypeAndValue(childtype.second, childvalue, fp, callback);
            } else {
                cout << "Aborting for i,key: " << i << ", " << key << endl;
                return ;
            }
            fp.pop_back();
        }
    }
    else if (type->IsUnion()) {
        const UnionValue *uv = dynamic_cast<const UnionValue *>(root);
        assert(uv != nullptr && "Expected Union Value");
        if (uv->Tag() <= type->ChildCount()) {
            // Ensure value's tag is not in a forward version than type so ignore
            const auto childtype = type->GetChild(uv->Tag());
            const auto childvalue = uv->Data().get();
            auto &key = childtype.first;
            fp.push_back(key);
            if (callback(childtype.second, childvalue, uv->Tag(), &key, fp)) {
                MatchTypeAndValue(childtype.second, childvalue, fp, callback);
            } else {
                cout << "Aborting for tag,key: " << uv->Tag() << ", " << key << endl;
                return ;
            }
            fp.pop_back();
        }
    } else {    // type function
        // Any matching will need to be applied by the caller
        // as they know what to do with these types
#if 0
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
#endif
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

void MatchTypeAndValue(const Type *type, const Value *value, MatchTypeAndValueCallback callback) {
    FieldPath fp;
    MatchTypeAndValue(type, value, fp, callback);
}

END_NS
