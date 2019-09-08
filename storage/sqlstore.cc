
#include "storage/storage.h"

START_NS

SQLStore::SQLStore(const string &path) : db(new SQLDB(path)) {
}

shared_ptr<SQLCollection> SQLStore::GetCollection(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = make_shared<SQLCollection>(schema, db);
    }
    return tables[schema];
}

SQLCollection::SQLCollection(const Schema *s, shared_ptr<SQLDB> db_) : Collection(s), db(db_) {
    // What other "tables" do we need?
    base_table = db->EnsureTable(schema);
}

Value *SQLCollection::Get(const Value &key) {
    auto table = base_table.lock();
    return table->Get(key);
}

bool SQLCollection::Put(Value &entity) {
    auto table = base_table.lock();
    return table->Put(&entity);
}

bool SQLCollection::DeleteByKey(const Value &key) {
    return false;
}

END_NS
