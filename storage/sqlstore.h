
// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQLSTORE_H
#define SQLSTORE_H

#include "storage/store.h"
#include "storage/sqldb.h"

START_NS

class SQLCollection : public Collection {
public:
    using Collection::Delete;
    SQLCollection(const Schema *s, shared_ptr<SQLDB> db_);
    virtual StrongValue Get(StrongValue key);
    virtual bool Put(StrongValue entity);
    virtual bool Delete(StrongValue key);

protected:
    shared_ptr<SQLDB> db;
    weak_ptr<SQLTable> base_table;
};

class SQLStore : public Store {
public:
    SQLStore(const string &dbpath);
    virtual shared_ptr<Collection> GetCollection(const Schema *t);

private:
    map<const Schema *, shared_ptr<SQLCollection>> tables;
    shared_ptr<SQLDB> db;
};

END_NS

#endif
