// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  disk input/output routines */

/*
 *  Fileread(p, num, file, posn, routine); -- generic file read
 *  Filewrite(p, num, file, posn, routine); -- generic file write
 *
 */

import std.compat;

#include "gb/files_rw.h"

#include <sys/file.h>
#include <unistd.h>

import std;

void Fileread(int fd, char *p, size_t num, int posn) {
  if (lseek(fd, posn, L_SET) < 0) {
    perror("Fileread 1");
    return;
  }
  if ((read(fd, p, num)) != num) {
    perror("Fileread 2");
  }
}

void Filewrite(int fd, const char *p, size_t num, int posn) {
  if (lseek(fd, posn, L_SET) < 0) {
    perror("Filewrite 1");
    return;
  }

  if ((write(fd, p, num)) != num) {
    perror("Filewrite 2");
    return;
  }
}
