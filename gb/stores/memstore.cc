
#include "memstore.h"

MemStore::MemStore() {
}

shared_ptr<MemTable> MemStore::GetTable(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = make_shared<MemTable>(schema);
    }
    return tables[schema];
}

Entity *MemTable::Get(const Value &key) {
    return entries[&key];
}

void MemTable::Put(Entity &entity) {
    entries[entity.GetKey()] = &entity;
}

void MemTable::Delete(const Value &key) {
    entries.erase(&key);
}

MemTable::MemTable(const Schema *schema_) : schema(schema_) {
    // TODO: Now create the necessary constraint structures so they can be honored in our CRUDs
}
