
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLDB_H
#define SQLDB_H

#include <sqlite3.h>
#include "storage/types.h"

START_NS

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
    shared_ptr<SQLTable> EnsureTable(const Schema *s);
    void CloseStatement(sqlite3_stmt *&stmt);
    sqlite3_stmt *PrepareSql(const string &sql_str);

protected:
    shared_ptr<SQLTable> processSchema(const Schema *s);

private:
    map<string, shared_ptr<SQLTable>> tables;
    string dbpath;
    int db_status;
    sqlite3 *dbhandle;
    string last_error;
    bool log_queries = true;
};

/**
 * A physical SQL table to help map between physical table and logical schemas we have.
 */
class SQLTable {
public:
    struct Column {
        const Type *coltype;
        bool is_pkey = false;
        bool required = false;
        Value *default_read_value = nullptr;
        Value *default_write_value = nullptr;
        const string &Name() const { return name; }
        const FieldPath &FP() const { return field_path; }
        const Type *GetType() const { return coltype; }
    private:
        string name;
        FieldPath field_path;
        int index;
        int endIndex;
        friend SQLTable ;
    };

public:
    SQLTable(SQLDB *db_, const string &name, const Schema *s);

    /**
     * Ensure the table has been created.
     */
    bool EnsureTable();
    bool Put(Value *entity) const;
    bool Delete(const Value *key) const;
    Value *Get(const Value &key) const;

    /**
     * Returns true if we have a physical column with the given name
     */
    bool HasColumn(const string &name) const;
    size_t ColCount() const { return columns.size(); } 
    const Column *AddColumn(const FieldPath &fp, const Type *t);
    const Column *ColumnAt(size_t index) const;
    const Column *ColumnFor(const string &name) const;
    const Column *ColumnFor(const FieldPath &fp) const;
    const string &Name() const { return table_name; }

    string TableCreationSQL() const;
    string InsertionSQL(const Value *value) const;
    string UpsertionSQL(const Value *key, const Value *value) const;
    string DeletionSQL(const Value *key) const;
    string GetSQL(const Value &key) const;

protected:
    void processType(const Type *t, FieldPath &fp);
    Value *resultSetToValue(sqlite3_stmt *stmt, bool is_root, const Type *currType,
                            int startCol, int endCol) const;

private: 
    SQLDB *db;
    string table_name;
    const Schema *schema;
    mutable map<FieldPath, Column *> columns_by_fp;
    mutable map<string, Column *> columns_by_name;
    vector <Column *> columns;
    bool table_created = false;

    string joinedColNamesFor(const list <FieldPath> &field_paths) const;
};

END_NS

#endif
