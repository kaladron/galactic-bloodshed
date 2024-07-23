// SPDX-License-Identifier: Apache-2.0

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

import gblib;
import std.compat;

#include "gb/sql/sql.h"

#include <sqlite3.h>

#include <cstdio>

#include "gb/sql/dbdecl.h"

Sql::Sql() {
  int err = sqlite3_open(PKGSTATEDIR "gb.db", &dbconn);
  if (err) {
    std::println(stderr, "Can't open database: {0}", sqlite3_errmsg(dbconn));
    exit(0);
  }

  open_files();
}

Sql::~Sql() { close_files(); }
