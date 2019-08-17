
#include "storage/storage.h"
#include <sstream>

START_NS

FieldPath::FieldPath(const string &input, const string &delim) {
    splitString(input, delim, *this);
}

FieldPath::FieldPath(const vector<string> &another) : vector<string>(another) {
}

FieldPath FieldPath::push(const string &subfield) {
    FieldPath out(*this);
    out.push_back(subfield);
    return out;
}

string FieldPath::join(const string &delim) const {
    return joinStrings(*this, delim);
}

Type::Type(const string &name_) : name(name_), type_tag(TYPE_FUN) {
}

Type::Type(const string &name_, const TypeVector &args) : name(name_) {
    SetData(args);
}

Type::Type(const string &name_, const NameTypeVector &fields, bool is_product_type) : name(name_) {
    SetData(fields, is_product_type);
}

Type::~Type() {
    Clear();
}

void Type::Clear() {
    child_names.clear();
    child_types.clear();
}

void Type::SetData(const TypeVector &args) {
    Clear();
    type_tag = TYPE_FUN;
    for (auto t : args) {
        AddChild(t);
    }
}

void Type::SetData(const NameTypeVector &fields, bool is_product_type) {
    Clear();
    type_tag = is_product_type ? RECORD : UNION;
    for (auto t : fields) {
        AddChild(t.second, t.first);
    }
}

size_t Type::ChildCount() const {
    return child_types.size();
}

NameTypePair Type::GetChild(size_t index) const {
    if (type_tag == RECORD || type_tag == UNION) {
        return NameTypePair(child_names.at(index), child_types.at(index));
    } else {
        return NameTypePair("", child_types.at(index));
    }
}

Type *Type::GetChild(const string &name) const {
    auto it = std::find(child_names.begin(), child_names.end(), name);
    if (it == child_names.end())
        return nullptr;

    size_t index = std::distance(child_names.begin(), it);
    return child_types.at(index);
}

void Type::AddChild(Type *child, const string &name) {
    // Only compound types can have names
    if (type_tag == RECORD || type_tag == UNION) {
        child_names.push_back(name);
    }
    AddChild(child);
}

int Type::Compare(const Type &another) const {
    if (this == &another) return 0;
    if (type_tag != another.type_tag) return type_tag - another.type_tag;
    int cmp = name.compare(another.name);
    if (cmp != 0) return cmp;

    // compare names
    cmp = IterCompare(
            child_names.begin(), child_names.end(),
            another.child_names.begin(), another.child_names.end(),
            [](const auto &a, const auto &b) { return a.compare(b); });
    if (cmp != 0) return cmp;

    // Now compare child values - note we dont do a deep compare as
    // types will often be created once and referenced everywhere!
    return IterCompare(
            child_types.begin(), child_types.end(),
            another.child_types.begin(), another.child_types.end(),
            [](const auto &a, const auto &b) { return a - b; });
}

const Type *CharType = new Type("char");
const Type *BoolType = new Type("bool");
const Type *IntType = new Type("int");
const Type *FloatType = new Type("float");
const Type *LongType = new Type("long");
const Type *DoubleType = new Type("double");

END_NS