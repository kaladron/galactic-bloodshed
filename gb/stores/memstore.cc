
#include "memstore.h"

MemStore::MemStore() {
}

shared_ptr<MemTable> MemStore::GetTable(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = make_shared<MemTable>();
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
