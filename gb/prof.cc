// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* prof.c -- print out racial profile */

import gblib;
import std;

#include "gb/prof.h"

namespace {
static int round_perc(const int data, const Race &r, int p) {
  int k = 101 - MIN(r.translate[p - 1], 100);
  return ((data / k) * k);
}
}  // namespace

std::string Estimate_f(const double data, const Race &r, const player_t p) {
  if (r.translate[p - 1] > 10) {
    int est = round_perc((int)data, r, p);
    if (est < 1000)
      return std::format("{}", est);
    else if (est < 10000)
      return std::format("{:.1f}K", static_cast<double>(est) / 1000.);
    else if (est < 1000000)
      return std::format("{:.0f}K", static_cast<double>(est) / 1000.);
    else
      return std::format("{:.1f}M", static_cast<double>(est) / 1000000.);
  }
  return "?";
}

std::string Estimate_i(const int data, const Race &r, const player_t p) {
  if (r.translate[p - 1] > 10) {
    int est = round_perc((int)data, r, p);
    if (std::abs(est) < 1000)
      return std::format("{}", est);
    else if (std::abs(est) < 10000)
      return std::format("{:.1f}K", static_cast<double>(est) / 1000.);
    else if (std::abs(est) < 1000000)
      return std::format("{:.0f}K", static_cast<double>(est) / 1000.);
    else
      return std::format("{:.1f}M", static_cast<double>(est) / 1000000.);
  }
  return "?";
}
