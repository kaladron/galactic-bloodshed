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
  virtual int Numcommods();
  virtual int Numraces();
  virtual void putcommod(Commod *, int);
  virtual void putship(Ship *);
  virtual void putstar(startype *, starnum_t);
  virtual void putrace(Race *);
  virtual void putsdata(struct stardata *);
  virtual void getsdata(struct stardata *);
  virtual void getrace(Race **, int);
  virtual void getstar(startype **, int);
  virtual std::optional<Ship> getship(const shipnum_t shipnum);
  virtual std::optional<Ship> getship(Ship **, const shipnum_t);
  virtual int getcommod(Commod **, commodnum_t);
  virtual Planet getplanet(const starnum_t, const planetnum_t);
  virtual void putplanet(const Planet &, Star *, const int);
};

#endif  // SQL_SQL_H
