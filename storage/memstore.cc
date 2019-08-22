
#include "storage/storage.h"

START_NS

std::shared_ptr<MemCollection> MemStore::GetCollection(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = std::make_shared<MemCollection>(schema);
    }
    return tables[schema];
}

bool MemCollection::Get(const Value &key, Value &result) {
    auto it = entries.find(&key);
    if (it == entries.end()) return false;
    result = *(it->second);
    return true;
}

void MemCollection::Put(Value &entity) {
    entries[schema->GetKey(entity)] = &entity;
}

void MemCollection::DeleteByKey(const Value &key) {
    entries.erase(&key);
}

MemCollection::MemCollection(const Schema *schema_) : Collection(schema_) {
    // TODO: Now create the necessary constraint structures so they can be honored in our CRUDs
}

END_NS
