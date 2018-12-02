// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file zoom.cc
/// \brief Functions for implementing the 'zoom' command.

#include "zoom.h"

#include <stdio.h>
#include <boost/format.hpp>
#include <string>

#include "GB_server.h"
#include "vars.h"

/// Zoom in or out for orbit display
void zoom(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int i = (Dir[Playernum - 1][Governor].level == ScopeLevel::LEVEL_UNIV);

  if (argv.size() > 1) {
    double num, denom;
    if (sscanf(argv[1].c_str(), "%lf/%lf", &num, &denom) == 2) {
      /* num/denom format */
      if (denom == 0.0) {
        std::string outmsg = "Illegal denominator value.\n";
        notify(Playernum, Governor, outmsg);
      } else
        g.zoom[i] = num / denom;
    } else {
      /* one number */
      g.zoom[i] = num;
    }
  }

  std::string outmsg =
      str(boost::format("Zoom value %g, lastx = %g, lasty = %g.\n") %
          g.zoom[i] % g.lastx[i] % g.lasty[i]);
  notify(Playernum, Governor, outmsg);
}
