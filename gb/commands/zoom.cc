// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file zoom.cc
/// \brief Functions for implementing the 'zoom' command.

module commands;

import gblib;
import std;

/// Zoom in or out for orbit display
void zoom(const command_t &argv, GameObj &g) {
  int i = (g.level == ScopeLevel::LEVEL_UNIV);

  if (argv.size() > 1) {
    double num;
    double denom;
    if (sscanf(argv[1].c_str(), "%lf/%lf", &num, &denom) == 2) {
      /* num/denom format */
      if (denom == 0.0) {
        g.out << "Illegal denominator value.\n";
      } else
        g.zoom[i] = num / denom;
    } else {
      /* one number */
      g.zoom[i] = num;
    }
  }

  g.out << std::format("Zoom value {0}, lastx = {1}, lasty = {2}.\n", g.zoom[i],
                       g.lastx[i], g.lasty[i]);
}
