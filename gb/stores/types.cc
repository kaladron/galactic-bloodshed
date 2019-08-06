
#include "gb/stores/types.h"

Type::Type(const string &name_, TypeVector *args) : name(name_) {
    SetData(args);
}

Type::Type(const string &name_, NameTypeVector *fields, bool is_product_type) : name(name_) {
    SetData(fields, is_product_type);
}

Type::~Type() {
    Clear();
}

void Type::Clear() {
    if (child_names != nullptr) {
        delete child_names;
        child_names = nullptr;
    }
    if (child_types != nullptr) {
        delete child_types;
        child_types = nullptr;
    }
}

void Type::SetData(TypeVector *args) {
    Clear();
    type_tag = TYPE_FUN;
    for (auto t : *args) {
        AddChild(t);
    }
}

void Type::SetData(NameTypeVector *fields, bool is_product_type) {
    Clear();
    type_tag = is_product_type ? RECORD : UNION;
    for (auto t : *fields) {
        AddChild(t.first, t.second);
    }
}

size_t Type::ChildCount() const {
    return child_types->size();
}

NameTypePair Type::GetChild(size_t index) const {
    return NameTypePair(child_names->at(index), child_types->at(index));
}

Type *Type::GetChild(const string &name) const {
    if (child_names == nullptr)
        return nullptr;

    auto it = std::find(child_names->begin(), child_names->end(), name);
    if (it == child_names->end())
        return nullptr;

    size_t index = std::distance(child_names->begin(), it);
    return child_types->at(index);
}

void Type::AddChild(Type *child) {
    child_types->push_back(child);
}

void Type::AddChild(const string &name, Type *child) {
    if (type_tag == RECORD || type_tag == UNION) {
        child_names->push_back(name);
    }
    AddChild(child);
}


const Type *CharType = new Type("char");
const Type *BoolType = new Type("bool");
const Type *IntType = new Type("int");
const Type *FloatType = new Type("float");
const Type *LongType = new Type("long");
const Type *DoubleType = new Type("double");
