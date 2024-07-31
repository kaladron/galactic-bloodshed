// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SQL_SQL_H
#define SQL_SQL_H

#include "gb/files_shl.h"

class Sql : public Db {
 public:
  Sql();
  virtual ~Sql() override;
  virtual int Numcommods() override;
  virtual int Numraces() override;
  virtual void putcommod(const Commod &, int);
  virtual void putship(Ship *);
  virtual void putstar(const Star &, starnum_t);
  virtual void putrace(const Race &);
  virtual void putsdata(stardata *);
  virtual void getsdata(stardata *);
  virtual Race getrace(player_t);
  virtual Star getstar(starnum_t);
  virtual std::optional<Ship> getship(const shipnum_t shipnum);
  virtual std::optional<Ship> getship(Ship **, const shipnum_t);
  virtual Commod getcommod(commodnum_t);
  virtual Planet getplanet(const starnum_t, const planetnum_t);
  virtual void putplanet(const Planet &, const Star &, const planetnum_t);
};

#endif  // SQL_SQL_H
