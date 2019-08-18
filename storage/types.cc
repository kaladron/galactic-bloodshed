
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

Type::Type(const string &fqn_) : fqn(fqn_), type_tag(TYPE_FUN) {
}

Type::Type(const string &fqn_, const TypeVector &args) : fqn(fqn_) {
    SetData(args);
}

Type::Type(const string &fqn_, const NameTypeVector &fields, bool is_product_type) : fqn(fqn_) {
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

const Type *Type::GetChild(const string &name) const {
    auto it = std::find(child_names.begin(), child_names.end(), name);
    if (it == child_names.end())
        return nullptr;

    size_t index = std::distance(child_names.begin(), it);
    return child_types.at(index);
}

void Type::AddChild(const Type *child, const string &name) {
    // Only compound types can have names
    if (child != nullptr) {
        if (type_tag == RECORD || type_tag == UNION) {
            child_names.push_back(name);
        }
        child_types.push_back(child);
    }
}

int Type::Compare(const Type &another) const {
    if (this == &another) return 0;
    if (type_tag != another.type_tag) return type_tag - another.type_tag;
    int cmp = fqn.compare(another.fqn);
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

#define make_basic_type(T)          \
    static Type *outtype = nullptr; \
    if (outtype == nullptr) {       \
        outtype = new Type(T);      \
    }                               \
    return outtype;

const Type *DefaultTypes::CharType() {
    make_basic_type("char");
}

const Type *DefaultTypes::BoolType() {
    make_basic_type("bool");
}

const Type *DefaultTypes::IntType() {
    make_basic_type("int");
}

const Type *DefaultTypes::FloatType() {
    make_basic_type("float");
}

const Type *DefaultTypes::LongType() {
    make_basic_type("long");
}

const Type *DefaultTypes::DoubleType() {
    make_basic_type("double");
}

const Type *DefaultTypes::StringType() {
    make_basic_type("string");
}

END_NS
