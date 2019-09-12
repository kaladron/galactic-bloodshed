
#include "storage/storage.h"

START_NS

std::shared_ptr<MemCollection> MemStore::GetCollection(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = std::make_shared<MemCollection>(schema);
    }
    return tables[schema];
}

StrongValue MemCollection::Get(StrongValue key) {
    auto it = entries.find(key);
    if (it == entries.end()) return nullptr;
    return it->second;
}

bool MemCollection::Put(StrongValue entity) {
    auto entityptr = entity.get();
    if (!entityptr) {
        return false;
    }
    auto key = schema->GetKey(*entityptr);
    entries[key] = entity;
    return true;
}

bool MemCollection::Delete(StrongValue key) {
    entries.erase(key);
    return true;
}

MemCollection::MemCollection(const Schema *schema_) : Collection(schema_) {
    // TODO: Now create the necessary constraint structures so they can be honored in our CRUDs
}

END_NS
