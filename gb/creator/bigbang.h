// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef BIGBANG_H
#define BIGBANG_H

#include <memory>
#include <utility>
#include "gb/creator/namegen.h"

using namespace std;

class Universe {};

/**
 * Every universe begins with a Big Bang!!!
 */
class BigBang {
 public:
  Universe *go();
  // void place_star(startype *);

  void setPlanetNameGen(unique_ptr<NameGenerator> namegen);
  void setStarNameGen(unique_ptr<NameGenerator> namegen);

 private:
  unique_ptr<NameGenerator> star_name_gen;
  unique_ptr<NameGenerator> planet_name_gen;
  int minplanets = -1;
  int maxplanets = -1;
  int printplaninfo = 0;
  int printstarinfo = 0;
};

#endif  // MAKEUNIV_H
