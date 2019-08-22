
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

SQLCollection::SQLCollection(const Schema *s, shared_ptr<SQLDB> db_) 
    : Collection(s), db(db_),
    base_table(db->EnsureTable(schema)) {
    // What other "tables" do we need?
}

bool SQLCollection::Get(const Value &key, Value &result) {
    // return base_table->Get(key);
}

void SQLCollection::Put(Value &entity) {
    // return base_table->Put(entity);
}

void SQLCollection::DeleteByKey(const Value &key) {
}

END_NS
