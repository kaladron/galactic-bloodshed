// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

import gblib;
import std.compat;

#include <stdio.h>

#include "gb/sql/sql.h"

#include <fcntl.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gb/files.h"
#include "gb/power.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/sql/dbdecl.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

Sql::Sql() {
  int err = sqlite3_open(PKGSTATEDIR "gb.db", &dbconn);
  if (err) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(dbconn));
    exit(0);
  }

  open_files();
}

Sql::~Sql() { close_files(); }
