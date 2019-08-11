
#include <iostream>
#include <sstream>
#include <fmt/ostream.h>
#include "gb/stores/sqlstore.h"

SQLStore::SQLStore(const string &path) : db(new SQLDB(path)) {
}

shared_ptr<SQLCollection> SQLStore::GetCollection(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = make_shared<SQLCollection>(db, schema);
    }
    return tables[schema];
}

SQLCollection::SQLCollection(shared_ptr<SQLDB> db_, const Schema *s) 
    : db(db_),
    schema(s),
    base_table(db->EnsureTable(schema)) {
    // What other "tables" do we need?
}

Entity *SQLCollection::Get(const Value &key) {
    // return base_table->Get(key);
}

void SQLCollection::Put(Entity &entity) {
    // return base_table->Put(entity);
}

void SQLCollection::Delete(const Value &key) {
}
