// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* name.c -- rename something to something else */

#include "gb/commands/announce.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/dissolve.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/getplace.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

void announce(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  enum class Communicate {
    ANN,
    BROADCAST,
    SHOUT,
    THINK,
  };

  Communicate mode;
  if (argv[0] == "announce")
    mode = Communicate::ANN;
  else if (argv[0] == "broadcast")
    mode = Communicate::BROADCAST;
  else if (argv[0] == "'")
    mode = Communicate::BROADCAST;
  else if (argv[0] == "shout")
    mode = Communicate::SHOUT;
  else if (argv[0] == "think")
    mode = Communicate::THINK;
  else {
    g.out << "Not sure how you got here.\n";
    return;
  }

  racetype *Race;
  char symbol;

  Race = races[Playernum - 1];
  if (mode == Communicate::SHOUT && !Race->God) {
    g.out << "You are not privileged to use this command.\n";
    return;
  }

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (mode == Communicate::ANN) mode = Communicate::BROADCAST;
      break;
    default:
      if ((mode == Communicate::ANN) &&
          !(!!isset(Stars[g.snum]->inhabited, Playernum) || Race->God)) {
        g.out << "You do not inhabit this system or have diety privileges.\n";
        return;
      }
  }

  switch (mode) {
    case Communicate::ANN:
      symbol = ':';
      break;
    case Communicate::BROADCAST:
      symbol = '>';
      break;
    case Communicate::SHOUT:
      symbol = '!';
      break;
    case Communicate::THINK:
      symbol = '=';
      break;
  }
  char msg[1000];
  sprintf(msg, "%s \"%s\" [%d,%d] %c %s\n", Race->name,
          Race->governor[Governor].name, Playernum, Governor, symbol,
          message.c_str());

  switch (mode) {
    case Communicate::ANN:
      d_announce(Playernum, Governor, g.snum, msg);
      break;
    case Communicate::BROADCAST:
      d_broadcast(Playernum, Governor, msg);
      break;
    case Communicate::SHOUT:
      d_shout(Playernum, Governor, msg);
      break;
    case Communicate::THINK:
      d_think(Playernum, Governor, msg);
      break;
  }
}
