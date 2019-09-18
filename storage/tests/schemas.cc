
#include <gtest/gtest.h>
#include "storage/storage.h"

using namespace Storage;

// How do we want to test schemas?
// 1. Define a schema - say from a string or a file
// 2. Do write/read/update/delete/get tests

/*
template <typename T>
shared_ptr<T> setupTestDb(const char *dbpath = "/tmp/storage.tests.db");

shared_ptr<Store> setupTestDb(const char *dbpath) {
    return make_shared<SQLStore>(dbpath);
}

TEST(Schemas, SimpleSchemas) {
    const char *schema = R"SCHEMA(
    )SCHEMA";
}
*/
