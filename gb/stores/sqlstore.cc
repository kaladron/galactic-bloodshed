
#include <iostream>
#include <sstream>
#include <fmt/ostream.h>
#include "sqlstore.h"

SQLStore::SQLStore(const string &path) : dbpath(path), dbhandle(nullptr) {
    db_status = sqlite3_open_v2(dbpath.c_str(), &dbhandle,
                                SQLITE_OPEN_CREATE |
                                    SQLITE_OPEN_READWRITE | 
                                    SQLITE_OPEN_SHAREDCACHE,
                                nullptr);
}

shared_ptr<SQLTable> SQLStore::GetTable(const Schema *schema) {
    if (tables.find(schema) == tables.end()) {
        tables[schema] = make_shared<SQLTable>(dbhandle, schema);
    }
    return tables[schema];
}

SQLTable::SQLTable(sqlite3 *dbh, const Schema *s) 
    : dbhandle(dbh), schema(s) {
    ensureTable();
}

bool SQLTable::ensureTable() {
    if (table_created) return true;

    stringstream sql;
    string table_name(schema->Name());
    Type *entity_type = schema->EntityType();
    Type *key_type = schema->KeyType();
    int nFields = entity_type->ChildCount();
    int nKeyFields = key_type->ChildCount();

    sql << "CREATE TABLE IF NOT EXIST '" << table_name << "' (" << endl;
    for (int i = 0;i < nFields;i++) {
        NameTypePair field = entity_type->GetChild(i);
        string fname = field.first;
        Type *ftype = field.second;
        if (i > 0) sql << ", ";
        sql << fname << " " << endl;
        // TODO - See how to generically:
        // 1. pass constraints
        // 2. pass default values
        if (ftype->Name() == "int") {
            sql << "INT" << " " << endl;
        } else if (ftype->Name() == "long") {
            sql << "INT64" << " " << endl;
        } else if (ftype->Name() == "double") {
            sql << "DOUBLE" << " " << endl;
        } else if (ftype->Name() == "string") {
            sql << "TEXT" << " " << endl;
        } else if (ftype->Name() == "datetime") {
            sql << "DATETIME" << " " << endl;
        } else {
            assert(false && "Invalid child type");
        }
    }
    sql << ")" << endl;

    sqlite3_stmt *stmt = prepareSql(sql.str());
    if (stmt == nullptr) return false;

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        last_error = string("Could not create table (%@): ") +
                        table_name + string(sqlite3_errmsg(dbhandle));
        return false;
    }
    closeStatement(stmt);

    table_created = true;
    return true;
}

void SQLTable::closeStatement(sqlite3_stmt *&stmt) {
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    stmt=NULL;
}

sqlite3_stmt *SQLTable::prepareSql(const string &sql_str) {
    sqlite3_stmt *stmt = NULL;
    int result = sqlite3_prepare_v2(dbhandle, sql_str.c_str(), -1, &stmt, NULL);
    last_query = sql_str;
    if (log_queries)
    {
        cout << "Prepared Sql: " << sql_str << endl;
    }
    if (result != SQLITE_OK)
    {
        last_error = string("Could not prepare sql: ") + string(sqlite3_errmsg(dbhandle));
        return nullptr;
    }
    return stmt;
}

Entity *SQLTable::Get(const Value &key) {
}

void SQLTable::Put(Entity &entity) {
}

void SQLTable::Delete(const Value &key) {
}

