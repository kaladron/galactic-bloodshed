
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLSTORE_H
#define SQLSTORE_H

#include "store.h"
#include <sqlite3.h>

class SQLTable : public Table {
public:
    SQLTable(sqlite3 *dbhandle);
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);

protected:
    sqlite3 *dbhandle;
};

class SQLStore : public Store<SQLTable> {
public:
    SQLStore(const string &dbpath);
    virtual shared_ptr<SQLTable> GetTable(const Schema *t);

private:
    string dbpath;
    sqlite3 *dbhandle;
    map<const Schema *, shared_ptr<SQLTable>> tables;
};

#endif
