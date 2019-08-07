
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLSTORE_H
#define SQLSTORE_H

#include "store.h"
#include <sqlite3.h>

class SQLTable : public Table {
public:
    SQLTable(sqlite3 *dbhandle, const Schema *t);
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);
    const string &LastError() { return last_error; }
    const string &LastQuery() { return last_query; }

protected:
    /**
     * Ensures that tables have been created.
     */
    bool ensureTable();

    /**
     * Helper to prepare an sql string and return a statement.
     */
    sqlite3_stmt *prepareSql(const string &sqlstr);

    /**
     * Closes and releases a prepared statement object.
     */
    void closeStatement(sqlite3_stmt *&stmt);

protected:
    sqlite3 *dbhandle;
    const Schema *schema;
    bool table_created = false;
    bool log_queries = true;
    string last_error;
    string last_query;
};

class SQLStore : public Store<SQLTable> {
public:
    SQLStore(const string &dbpath);
    virtual shared_ptr<SQLTable> GetTable(const Schema *t);

private:
    map<const Schema *, shared_ptr<SQLTable>> tables;
    string dbpath;
    int db_status;
    sqlite3 *dbhandle;
};

#endif
