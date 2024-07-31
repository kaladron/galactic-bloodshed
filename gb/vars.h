// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  main bunch of variables */

#ifndef VARS_H
#define VARS_H

#include "gb/tweakables.h"

/* number of movement segments (global variable) */
extern unsigned long segments;

#define M_FUEL 0x1
#define M_DESTRUCT 0x2
#define M_RESOURCES 0x4
#define M_CRYSTALS 0x8
#define Fuel(x) ((x) & M_FUEL)
#define Destruct(x) ((x) & M_DESTRUCT)
#define Resources(x) ((x) & M_RESOURCES)
#define Crystals(x) ((x) & M_CRYSTALS)

class SectorMap {
 public:
  SectorMap(const Planet &planet) : maxx_(planet.Maxx), maxy_(planet.Maxy) {
    vec_.reserve(planet.Maxx * planet.Maxy);
  }

  //! Add an empty sector for every potential space.  Used for initialization.
  SectorMap(const Planet &planet, bool)
      : maxx_(planet.Maxx),
        maxy_(planet.Maxy),
        vec_(planet.Maxx * planet.Maxy) {}

  // TODO(jeffbailey): Should wrap this in a subclass so the underlying
  // vector isn't exposed to callers.
  auto begin() { return vec_.begin(); }
  auto end() { return vec_.end(); }

  Sector &get(const int x, const int y) {
    return vec_.at(static_cast<size_t>(x + (y * maxx_)));
  }
  void put(Sector &&s) { vec_.emplace_back(std::move(s)); }
  int get_maxx() { return maxx_; }
  int get_maxy() { return maxy_; }
  Sector &get_random();
  // TODO(jeffbailey): Don't expose the underlying vector.
  std::vector<std::reference_wrapper<Sector>>
  shuffle();  /// Randomizes the order of the SectorMap.

  SectorMap(SectorMap &) = delete;
  void operator=(const SectorMap &) = delete;
  SectorMap(SectorMap &&) = default;
  SectorMap &operator=(SectorMap &&) = default;

 private:
  SectorMap(const int maxx, const int maxy) : maxx_(maxx), maxy_(maxy) {}
  int maxx_;
  int maxy_;
  std::vector<Sector> vec_;
};

#endif  // VARS_H
