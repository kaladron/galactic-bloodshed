
#include "storage/storage.h"
#include <sstream>

START_NS

FieldPath::FieldPath(const string &input, const string &delim) {
    splitString(input, delim, *this);
}

FieldPath::FieldPath(const StringVector &another) : StringVector(another) {
}

FieldPath FieldPath::push(const string &subfield) {
    FieldPath out(*this);
    out.push_back(subfield);
    return out;
}

string FieldPath::join(const string &delim) const {
    return joinStrings(*this, delim);
}

Type::TypeContainer::TypeContainer(const TypeContainer &another)
    : is_named(another.is_named),
      child_names(another.child_names),
      child_types(another.child_types) {
}

Type::TypeContainer::TypeContainer(const TypeVector &args) {
    SetData(args);
}

Type::TypeContainer::TypeContainer(const NameTypeVector &fields) {
    SetData(fields);
}

Type::TypeContainer::TypeContainer(std::initializer_list<StrongType> types) 
    : TypeContainer(TypeVector(types)) { }

Type::TypeContainer::TypeContainer(std::initializer_list<NameTypePair> fields) 
    : TypeContainer(NameTypeVector(fields)) { }

Type::TypeContainer::~TypeContainer() {
    Clear();
}

void Type::TypeContainer::Clear() {
    is_named = false;
    child_names.clear();
    child_types.clear();
}

void Type::TypeContainer::SetData(const TypeVector &args) {
    Clear();
    is_named = false;
    for (auto t : args) {
        AddChild(t);
    }
}

void Type::TypeContainer::SetData(const NameTypeVector &fields) {
    Clear();
    is_named = true;
    for (auto t : fields) {
        AddChild(t.second, t.first);
    }
}

int Type::TypeContainer::Compare(const TypeContainer &another) const {
    // compare names
    int cmp = IterCompare(
            child_names.begin(), child_names.end(),
            another.child_names.begin(), another.child_names.end(),
            [](const auto &a, const auto &b) { return a.compare(b); });
    if (cmp != 0) return cmp;

    // Now compare child values - note we dont do a deep compare as
    // types will often be created once and referenced everywhere!
    return IterCompare(
            child_types.begin(), child_types.end(),
            another.child_types.begin(), another.child_types.end(),
            [](const auto &a, const auto &b) {
                return StrongType(a).get() - 
                       StrongType(b).get();
            });
}

size_t Type::TypeContainer::ChildCount() const {
    return child_types.size();
}

NameTypePair Type::TypeContainer::GetChild(size_t index) const {
    auto child_type = child_types.at(index).lock();
    if (is_named) {
        return pair(child_names.at(index), child_type);
    } else {
        return pair("", child_type);
    }
}

WeakType Type::TypeContainer::GetChild(const std::string &name) const {
    auto it = std::find(child_names.begin(), child_names.end(), name);
    if (it == child_names.end())
        return WeakType();

    size_t index = std::distance(child_names.begin(), it);
    return child_types.at(index);
}

void Type::TypeContainer::AddChild(StrongType child, const std::string &name) {
    // Only compound types can have names
    if (child) {
        if (is_named) {
            child_names.push_back(name);
        }
        child_types.push_back(child);
    }
}

Type::RefType::RefType(StrongType t) : target_type(t) {
}

int Type::RefType::Compare(const RefType &another) const {
    return target_type.lock()->Compare(*another.target_type.lock());
}

Type::Type(const string &fqn_, const ProductType &t) 
    : tag(PRODUCT_TYPE), fqn(fqn_), product_type(t) {
}

Type::Type(const string &fqn_, const SumType &t) 
    : tag(SUM_TYPE), fqn(fqn_), sum_type(t) {
}

Type::Type(const string &fqn_, const TypeFun &t)
    : tag(TYPE_FUN), fqn(fqn_), type_fun(t) {
}

Type::Type(const string &fqn_, const RefType &t) 
    : tag(REF_TYPE), fqn(fqn_), ref_type(t) {
}

Type::Type(const ProductType &t) 
    : tag(PRODUCT_TYPE), product_type(t) {
}

Type::Type(const SumType &t) 
    : tag(SUM_TYPE), sum_type(t) {
}

Type::Type(const TypeFun &t)
    : tag(TYPE_FUN), type_fun(t) {
}

Type::Type(const RefType &t) 
    : tag(REF_TYPE), ref_type(t) {
}

Type::Type(const string &fqn_) : tag(TYPE_FUN), fqn(fqn_), type_fun(TypeFun()) {
}

Type::~Type() {
}

size_t Type::ChildCount() const {
    switch(tag) {
        case PRODUCT_TYPE: return AsProductType().ChildCount();
        case SUM_TYPE: return AsSumType().ChildCount();
        case TYPE_FUN: return AsTypeFun().ChildCount();
        case REF_TYPE: return 0;
    }
    return 0;
}

WeakType Type::GetChild(const string &name) const {
    switch(tag) {
        case PRODUCT_TYPE: return AsProductType().GetChild(name);
        case SUM_TYPE: return AsSumType().GetChild(name);
        case TYPE_FUN: return AsTypeFun().GetChild(name);
        default: break;
    }
    return WeakType();
}

NameTypePair Type::GetChild(size_t index) const {
    switch(tag) {
        case PRODUCT_TYPE: return AsProductType().GetChild(index);
        case SUM_TYPE: return AsSumType().GetChild(index);
        case TYPE_FUN: return AsTypeFun().GetChild(index);
        default: break;
    }
    return NameTypePair();
}


int Type::Compare(const Type &another) const {
    if (this == &another) return 0;
    if (tag != another.tag) return tag - another.tag;
    int cmp = fqn.compare(another.fqn);
    if (cmp != 0) return cmp;

    switch(tag) {
        case PRODUCT_TYPE: return AsProductType().Compare(another.AsProductType());
        case SUM_TYPE: return AsSumType().Compare(another.AsSumType());
        case TYPE_FUN: return AsTypeFun().Compare(another.AsTypeFun());
        case REF_TYPE: return AsRefType().Compare(another.AsRefType());
    }
}

END_NS
