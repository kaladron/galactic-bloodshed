// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* enslave.c -- ENSLAVE the planet below. */

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void enslave(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  ap_t APcount = 2;
  int aliens = 0;
  int def = 0;
  int attack = 0;

  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno) return;
  auto s = getship(*shipno);

  if (!s) {
    return;
  }
  if (testship(*s, Playernum, Governor)) {
    return;
  }
  if (s->type != ShipType::STYPE_OAP) {
    g.out << std::format("This ship is not an {}.\n",
                         Shipnames[ShipType::STYPE_OAP]);
    return;
  }
  if (s->whatorbits != ScopeLevel::LEVEL_PLAN) {
    g.out << std::format("{} doesn't orbit a planet.\n", ship_to_string(*s));
    return;
  }
  if (!enufAP(Playernum, Governor, stars[s->storbits].AP[Playernum - 1],
              APcount)) {
    return;
  }
  auto p = getplanet(s->storbits, s->pnumorbits);
  if (p.info[Playernum - 1].numsectsowned == 0) {
    g.out << "You don't have a garrison on the planet.\n";
    return;
  }

  /* add up forces attacking, defending */
  attack = aliens = def = 0;
  for (auto i = 1; i < MAXPLAYERS; i++) {
    if (p.info[i - 1].numsectsowned && i != Playernum) {
      aliens = 1;
      def += p.info[i - 1].destruct;
    }
  }

  if (!aliens) {
    g.out << "There is no one else on this planet to enslave!\n";
    return;
  }

  auto &race = races[Playernum - 1];

  Shiplist shiplist(p.ships);
  for (auto s2 : shiplist) {
    if (s2.alive && s2.active) {
      if (p.info[s2.owner].numsectsowned && s2.owner != Playernum)
        def += s2.destruct;
      else if (s2.owner == Playernum)
        attack += s2.destruct;
    }
  }

  deductAPs(g, APcount, s->storbits);

  g.out << "\nFor successful enslavement this ship and the other ships here\n";
  g.out << "that are yours must have a weapons\n";
  g.out << "capacity greater than twice that the enemy can muster, including\n";
  g.out << "the planet and all ships orbiting it.\n";
  g.out << std::format("\nTotal forces bearing on {}:   {}\n",
                       prin_ship_orbits(*s), attack);

  std::stringstream telegram;
  telegram << std::format("ALERT!!!\n\nPlanet /{}/{}", stars[s->storbits].name,
                          stars[s->storbits].pnames[s->pnumorbits]);

  if (def <= 2 * attack) {
    p.slaved_to = Playernum;
    putplanet(p, stars[s->storbits], s->pnumorbits);

    /* send telegs to anyone there */
    telegram << std::format("ENSLAVED by {}!!\n", ship_to_string(*s));
    telegram << std::format(
        "All material produced here will be\n"
        "diverted to {} coffers.",
        race.name);

    g.out << "\nEnslavement successful.  All material produced here will\n";
    g.out << std::format("be diverted to {}.\n", race.name);
    g.out << std::format(
        "You must maintain a garrison of 0.1%% the population of the\n");
    g.out << std::format(
        "planet (at least {:.0f}); otherwise there is a 50% chance that\n",
        p.popn * 0.001);
    g.out << std::format("enslaved population will revolt.\n");
  } else {
    telegram << std::format("repulsed attempt at enslavement by {}!!\n",
                            ship_to_string(*s));
    telegram << std::format(
        "Enslavement repulsed, defense/attack Ratio : {} to {}.\n", def,
        attack);

    g.out << "Enslavement repulsed.\n";
    g.out << "You needed more weapons bearing on the planet...\n";
  }

  for (auto i = 1; i < MAXPLAYERS; i++)
    if (p.info[i - 1].numsectsowned && i != Playernum)
      warn(i, stars[s->storbits].governor[i - 1], telegram.str());
}
}  // namespace GB::commands
