// SPDX-License-Identifier: Apache-2.0

/// \file database_impl.cppm
/// \brief Internal dallib partition holding the private SQLite connection
/// state.

module;

#include <sqlite3.h>

import std;

module dallib:impl;

/// Internal implementation struct holding the raw SQLite connection handle.
struct DatabaseImpl {
  sqlite3* conn = nullptr;

  ~DatabaseImpl() {
    if (conn) {
      sqlite3_close(conn);
      conn = nullptr;
    }
  }
};
