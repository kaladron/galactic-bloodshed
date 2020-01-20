// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* declare.c -- declare alliance, neutrality, war, the basic thing. */

import gblib;
import std;

#include "gb/declare.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/config.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

static void show_votes(int, int);

/* invite people to join your alliance block */
void invite(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  bool mode = argv[0] == "invite";

  int n;

  if (Governor) {
    g.out << "Only leaders may invite.\n";
    return;
  }
  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }
  auto& race = races[Playernum - 1];
  auto& alien = races[n - 1];
  if (mode) {
    setbit(Blocks[Playernum - 1].invite, n);
    sprintf(buf, "%s [%d] has invited you to join %s\n", race.name, Playernum,
            Blocks[Playernum - 1].name);
    warn_race(n, buf);
    sprintf(buf, "%s [%d] has been invited to join %s [%d]\n", alien.name, n,
            Blocks[Playernum - 1].name, Playernum);
    warn_race(Playernum, buf);
  } else {
    clrbit(Blocks[Playernum - 1].invite, n);
    sprintf(buf, "You have been blackballed from %s [%d]\n",
            Blocks[Playernum - 1].name, Playernum);
    warn_race(n, buf);
    sprintf(buf, "%s [%d] has been blackballed from %s [%d]\n", alien.name, n,
            Blocks[Playernum - 1].name, Playernum);
    warn_race(Playernum, buf);
  }
  post(buf, DECLARATION);

  Putblock(Blocks);
}

/* declare that you wish to be included in the alliance block */
void pledge(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  bool mode = argv[0] == "pledge";
  int n;

  if (Governor) {
    g.out << "Only leaders may pledge.\n";
    return;
  }
  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }
  auto& race = races[Playernum - 1];
  if (mode) {
    setbit(Blocks[n - 1].pledge, Playernum);
    sprintf(buf, "%s [%d] has pledged %s.\n", race.name, Playernum,
            Blocks[n - 1].name);
    warn_race(n, buf);
    sprintf(buf, "You have pledged allegiance to %s.\n", Blocks[n - 1].name);
    warn_race(Playernum, buf);

    switch (int_rand(1, 20)) {
      case 1:
        sprintf(
            buf,
            "%s [%d] joins the band wagon and pledges allegiance to %s [%d]!\n",
            race.name, Playernum, Blocks[n - 1].name, n);
        break;
      default:
        sprintf(buf, "%s [%d] pledges allegiance to %s [%d].\n", race.name,
                Playernum, Blocks[n - 1].name, n);
        break;
    }
  } else {
    clrbit(Blocks[n - 1].pledge, Playernum);
    sprintf(buf, "%s [%d] has quit %s [%d].\n", race.name, Playernum,
            Blocks[n - 1].name, n);
    warn_race(n, buf);
    sprintf(buf, "You have quit %s\n", Blocks[n - 1].name);
    warn_race(Playernum, buf);

    switch (int_rand(1, 20)) {
      case 1:
        sprintf(buf, "%s [%d] calls %s [%d] a bunch of geeks and QUITS!\n",
                race.name, Playernum, Blocks[n - 1].name, n);
        break;
      default:
        sprintf(buf, "%s [%d] has QUIT %s [%d]!\n", race.name, Playernum,
                Blocks[n - 1].name, n);
        break;
    }
  }

  post(buf, DECLARATION);

  compute_power_blocks();
  Putblock(Blocks);
}

void declare(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const int APcount = 1;
  int n;
  int d_mod;

  if (Governor) {
    g.out << "Only leaders may declare.\n";
    return;
  }

  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }

  /* look in sdata for APs first */
  /* enufAPs would print something */
  if ((int)Sdata.AP[Playernum - 1] >= APcount) {
    deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
    /* otherwise use current star */
  } else if ((g.level == ScopeLevel::LEVEL_STAR ||
              g.level == ScopeLevel::LEVEL_PLAN) &&
             enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1],
                    APcount)) {
    deductAPs(g, APcount, g.snum);
  } else {
    sprintf(buf, "You don't have enough AP's (%d)\n", APcount);
    notify(Playernum, Governor, buf);
    return;
  }

  auto& race = races[Playernum - 1];
  auto& alien = races[n - 1];

  switch (*argv[2].c_str()) {
    case 'a':
      setbit(race.allied, n);
      clrbit(race.atwar, n);
      if (success(5)) {
        sprintf(buf, "But would you want your sister to marry one?\n");
        notify(Playernum, Governor, buf);
      } else {
        sprintf(buf, "Good for you.\n");
        notify(Playernum, Governor, buf);
      }
      sprintf(buf, " Player #%d (%s) has declared an alliance with you!\n",
              Playernum, race.name);
      warn_race(n, buf);
      sprintf(buf, "%s [%d] declares ALLIANCE with %s [%d].\n", race.name,
              Playernum, alien.name, n);
      d_mod = 30;
      if (argv.size() > 3) d_mod = std::stoi(argv[3]);
      d_mod = std::max(d_mod, 30);
      break;
    case 'n':
      clrbit(race.allied, n);
      clrbit(race.atwar, n);
      sprintf(buf, "Done.\n");
      notify(Playernum, Governor, buf);

      sprintf(buf, " Player #%d (%s) has declared neutrality with you!\n",
              Playernum, race.name);
      warn_race(n, buf);
      sprintf(buf, "%s [%d] declares a state of neutrality with %s [%d].\n",
              race.name, Playernum, alien.name, n);
      d_mod = 30;
      break;
    case 'w':
      setbit(race.atwar, n);
      clrbit(race.allied, n);
      if (success(4)) {
        sprintf(buf,
                "Your enemies flaunt their secondary male reproductive "
                "glands in your\ngeneral direction.\n");
        notify(Playernum, Governor, buf);
      } else {
        sprintf(buf, "Give 'em hell!\n");
        notify(Playernum, Governor, buf);
      }
      sprintf(buf, " Player #%d (%s) has declared war against you!\n",
              Playernum, race.name);
      warn_race(n, buf);
      switch (int_rand(1, 5)) {
        case 1:
          sprintf(buf, "%s [%d] declares WAR on %s [%d].\n", race.name,
                  Playernum, alien.name, n);
          break;
        case 2:
          sprintf(buf, "%s [%d] has had enough of %s [%d] and declares WAR!\n",
                  race.name, Playernum, alien.name, n);
          break;
        case 3:
          sprintf(
              buf,
              "%s [%d] decided that it is time to declare WAR on %s [%d]!\n",
              race.name, Playernum, alien.name, n);
          break;
        case 4:
          sprintf(buf,
                  "%s [%d] had no choice but to declare WAR against %s [%d]!\n",
                  race.name, Playernum, alien.name, n);
          break;
        case 5:
          sprintf(buf,
                  "%s [%d] says 'screw it!' and declares WAR on %s [%d]!\n",
                  race.name, Playernum, alien.name, n);
          break;
        default:
          break;
      }
      d_mod = 30;
      break;
    default:
      g.out << "I don't understand.\n";
      return;
  }

  post(buf, DECLARATION);
  warn_race(Playernum, buf);

  /* They, of course, learn more about you */
  alien.translate[Playernum - 1] =
      MIN(alien.translate[Playernum - 1] + d_mod, 100);

  putrace(alien);
  putrace(race);
}

#ifdef VOTING
void vote(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int check;
  int nvotes;
  int nays;
  int yays;

  auto& race = races[Playernum - 1];

  if (race.God) {
    sprintf(buf, "Your vote doesn't count, however, here is the count.\n");
    notify(Playernum, Governor, buf);
    show_votes(Playernum, Governor);
    return;
  }
  if (race.Guest) {
    sprintf(buf, "You are not allowed to vote, but, here is the count.\n");
    notify(Playernum, Governor, buf);
    show_votes(Playernum, Governor);
    return;
  }

  if (argv.size() > 2) {
    check = 0;
    if (argv[1] == "update") {
      if (argv[2] == "go") {
        race.votes = true;
        check = 1;
      } else if (argv[2] == "wait")
        race.votes = false;
      else {
        sprintf(buf, "No such update choice '%s'\n", argv[2].c_str());
        notify(Playernum, Governor, buf);
        return;
      }
    } else {
      sprintf(buf, "No such vote '%s'\n", argv[1].c_str());
      notify(Playernum, Governor, buf);
      return;
    }
    putrace(race);

    if (check) {
      /* Ok...someone voted yes.  Tally them all up and see if */
      /* we should do something. */
      nays = 0;
      yays = 0;
      nvotes = 0;
      for (player_t pnum = 1; pnum <= Num_races; pnum++) {
        auto& r = races[pnum - 1];
        if (r.God || r.Guest) continue;
        nvotes++;
        if (r.votes)
          yays++;
        else
          nays++;
      }
      /* Is Update/Movement vote unanimous now? */
      if (nvotes > 0 && nvotes == yays && nays == 0) {
        /* Do it... */
        do_next_thing(g.db);
      }
    }
  } else {
    sprintf(buf, "Your vote on updates is %s\n", race.votes ? "go" : "wait");
    notify(Playernum, Governor, buf);
    show_votes(Playernum, Governor);
  }
}

static void show_votes(int Playernum, int Governor) {
  int nvotes;
  int nays;
  int yays;
  int pnum;

  nays = yays = nvotes = 0;
  for (pnum = 1; pnum <= Num_races; pnum++) {
    auto& race = races[pnum - 1];
    if (race.God || race.Guest) continue;
    nvotes++;
    if (race.votes) {
      yays++;
      sprintf(buf, "  %s voted go.\n", race.name);
    } else {
      nays++;
      sprintf(buf, "  %s voted wait.\n", race.name);
    }
    if (races[Playernum - 1].God) notify(Playernum, Governor, buf);
  }
  sprintf(buf, "  Total votes = %d, Go = %d, Wait = %d.\n", nvotes, yays, nays);
  notify(Playernum, Governor, buf);
}
#endif
