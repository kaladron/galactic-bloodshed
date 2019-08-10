
#include <iostream>
#include <sstream>
#include <fmt/ostream.h>
#include "gb/stores/sqldb.h"

SQLDB::SQLDB(const string &path) : dbpath(path), dbhandle(nullptr) {
    db_status = sqlite3_open_v2(dbpath.c_str(), &dbhandle,
                                SQLITE_OPEN_CREATE |
                                    SQLITE_OPEN_READWRITE | 
                                    SQLITE_OPEN_SHAREDCACHE,
                                nullptr);
}

weak_ptr<SQLTable> SQLDB::EnsureTable(const Schema *s) {
    // return nullptr;
}

void SQLDB::CloseStatement(sqlite3_stmt *&stmt) {
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    stmt=NULL;
}

sqlite3_stmt *SQLDB::PrepareSql(const string &sql_str) {
    sqlite3_stmt *stmt = NULL;
    int result = sqlite3_prepare_v2(dbhandle, sql_str.c_str(), -1, &stmt, NULL);
    // last_query = sql_str;
    if (log_queries) {
        cout << "Prepared Sql: " << sql_str << endl;
    }
    if (result != SQLITE_OK)
    {
        last_error = string("Could not prepare sql: ") + string(sqlite3_errmsg(dbhandle));
        return nullptr;
    }
    return stmt;
}


/********************************************************************/
/*                      All things SQLTable related.                */
/********************************************************************/

bool SQLTable::CreateTable() {
    string sql = CreationSQL();
    sqlite3_stmt *stmt = db->PrepareSql(sql);
    if (stmt == nullptr) return false;

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        // last_error = string("Could not create table (%@): ") + table_name + string(sqlite3_errmsg(dbhandle));
        return false;
    }
    db->CloseStatement(stmt);
    return true;
}

/**
 * Returns the sql to create this table.
 */
string SQLTable::CreationSQL() const {
    stringstream sql;
    sql << "CREATE TABLE IF NOT EXIST '" << table_name << "' (" << endl;

    for (auto column : columns) {
    }
    /*
    Type *entity_type = schema->EntityType();
    Type *key_type = schema->KeyType();
    int nFields = entity_type->ChildCount();
    int nKeyFields = key_type->ChildCount();

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

        // Get field constraints
        for (auto constraint : schema->GetConstraints()) {
        }
        sql << ";";
    }
    */
    sql << ")" << endl;
    return sql.str();
};
