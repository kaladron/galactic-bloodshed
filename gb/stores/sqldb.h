
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLDB_H
#define SQLDB_H

#include <sqlite3.h>
#include "gb/stores/schema.h"

class SQLTable;

/**
 * A wrapper ontop of a physical SQL DB with metadata about
 * the db, tables, views etc.
 * TODO: currently a wrapper ontop of sqlite but move out
 */
class SQLDB {
public:
    SQLDB(const string &dbpath);

public:
    weak_ptr<SQLTable> EnsureTable(const Schema *s);
    void CloseStatement(sqlite3_stmt *&stmt);
    sqlite3_stmt *PrepareSql(const string &sql_str);

private:
    string dbpath;
    int db_status;
    sqlite3 *dbhandle;
    string last_error;
    bool log_queries;
};

/**
 * A physical SQL table to help map between physical table and logical schemas we have.
 */
class SQLTable {
public:
    SQLTable(SQLDB *db_, const string &name) : db(db_), table_name(name) { }
    void AddField(const FieldPath &fp, Type *starting);
    void AddField(const string &fieldname, Type *starting);
    bool CreateTable();
    string CreationSQL() const;

private: 
    struct Column {
        string name;
        FieldPath field_path;
        const Type *coltype;
    };
    SQLDB *db;
    string table_name;
    map<FieldPath, const Column *> fp_to_column;
    map<string, const Column *> name_to_column;
    list <Column> columns;
    bool table_created = false;
};

#endif
