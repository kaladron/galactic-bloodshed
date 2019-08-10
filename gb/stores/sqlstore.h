
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLSTORE_H
#define SQLSTORE_H

#include "gb/stores/store.h"
#include "gb/stores/sqldb.h"

class SQLCollection : public Collection {
public:
    SQLCollection(shared_ptr<SQLDB> db_, const Schema *s) ;
    Entity *Get(const Value &key);
    void Put(Entity &entity);
    void Delete(const Value &key);

protected:
    const Schema *schema;
    shared_ptr<SQLDB> db;
    weak_ptr<SQLTable> base_table;
};

class SQLStore : public Store<SQLCollection> {
public:
    SQLStore(const string &dbpath);
    virtual shared_ptr<SQLCollection> GetCollection(const Schema *t);

private:
    map<const Schema *, shared_ptr<SQLCollection>> tables;
    shared_ptr<SQLDB> db;
};

#endif
