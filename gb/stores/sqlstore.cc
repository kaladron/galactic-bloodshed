
#include "sqlstore.h"

SQLStore::SQLStore(const string &path) : dbpath(path), dbhandle(nullptr) {
}

shared_ptr<SQLTable> SQLStore::GetTable(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = make_shared<SQLTable>(dbhandle);
    }
    return tables[schema];
}

SQLTable::SQLTable(sqlite3 *dbh) : dbhandle(dbh) {
}

Entity *SQLTable::Get(const Value &key) {
}

void SQLTable::Put(Entity &entity) {
}

void SQLTable::Delete(const Value &key) {
}

