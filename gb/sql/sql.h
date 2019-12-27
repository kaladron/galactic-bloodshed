// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQL_SQL_H
#define SQL_SQL_H

#include "gb/files_shl.h"

class Sql : public Db {
 public:
  Sql();
  virtual ~Sql();
  int Numcommods();
  int Numraces();
  void putcommod(commodtype *, int);
  void putship(Ship *);
  void putstar(startype *, starnum_t);
  void putrace(Race *);
  void putsdata(struct stardata *);
  void getsdata(struct stardata *);
  void getrace(Race **, int);
  void getstar(startype **, int);
  std::optional<Ship> getship(const shipnum_t shipnum);
  std::optional<Ship> getship(Ship **, const shipnum_t);
  int getcommod(commodtype **, commodnum_t);
};

#endif  // SQL_SQL_H
