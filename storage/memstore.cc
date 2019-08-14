
#include "storage/storage.h"

START_NS

MemStore::MemStore() {
}

std::shared_ptr<MemCollection> MemStore::GetCollection(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = std::make_shared<MemCollection>(schema);
    }
    return tables[schema];
}

Entity *MemCollection::Get(const Value &key) {
    return entries[&key];
}

void MemCollection::Put(Entity &entity) {
    entries[entity.GetKey()] = &entity;
}

void MemCollection::Delete(const Value &key) {
    entries.erase(&key);
}

MemCollection::MemCollection(const Schema *schema_) : schema(schema_) {
    // TODO: Now create the necessary constraint structures so they can be honored in our CRUDs
}

END_NS
