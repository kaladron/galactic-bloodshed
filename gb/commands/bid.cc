// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"
#include "gb/build.h"

module commands;

namespace GB::commands {
void bid(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  Planet p;
  char commod;
  int i;
  int item;
  int lot;
  double rate;
  int snum;
  int pnum;

  if (argv.size() == 1) {
    /* list all market blocks for sale */
    notify(Playernum, Governor,
           "+++ Galactic Bloodshed Commodities Market +++\n\n");
    notify(Playernum, Governor,
           "  Lot Stock      Type  Owner  Bidder  Amount "
           "Cost/Unit    Ship  Dest\n");
    for (i = 1; i <= g.db.Numcommods(); i++) {
      auto c = getcommod(i);
      if (c.owner && c.amount) {
        rate = (double)c.bid / (double)c.amount;
        if (c.bidder == Playernum)
          sprintf(temp, "%4.4s/%-4.4s", stars[c.star_to].name,
                  stars[c.star_to].pnames[c.planet_to]);
        else
          temp[0] = '\0';

        auto [cost, dist] = shipping_cost(c.star_from, g.snum, c.bid);
        sprintf(buf, " %4d%c%5lu%10s%7d%8d%8ld%10.2f%8ld %10s\n", i,
                c.deliver ? '*' : ' ', c.amount, commod_name[c.type], c.owner,
                c.bidder, c.bid, rate, cost, temp);
        notify(Playernum, Governor, buf);
      }
    }
  } else if (argv.size() == 2) {
    /* list all market blocks for sale of the requested type */
    commod = argv[1][0];
    switch (commod) {
      case 'r':
        item = RESOURCE;
        break;
      case 'd':
        item = DESTRUCT;
        break;
      case 'f':
        item = FUEL;
        break;
      case 'x':
        item = CRYSTAL;
        break;
      default:
        g.out << "No such type of commodity.\n";
        return;
    }
    g.out << "+++ Galactic Bloodshed Commodities Market +++\n\n";
    g.out << "  Lot Stock      Type  Owner  Bidder  Amount "
             "Cost/Unit    Ship  Dest\n";
    for (i = 1; i <= g.db.Numcommods(); i++) {
      auto c = getcommod(i);
      if (c.owner && c.amount && (c.type == item)) {
        rate = (double)c.bid / (double)c.amount;
        if (c.bidder == Playernum)
          sprintf(temp, "%4.4s/%-4.4s", stars[c.star_to].name,
                  stars[c.star_to].pnames[c.planet_to]);
        else
          temp[0] = '\0';
        auto [cost, dist] = shipping_cost(c.star_from, g.snum, c.bid);
        sprintf(buf, " %4d%c%5lu%10s%7d%8d%8ld%10.2f%8ld %10s\n", i,
                c.deliver ? '*' : ' ', c.amount, commod_name[c.type], c.owner,
                c.bidder, c.bid, rate, cost, temp);
        notify(Playernum, Governor, buf);
      }
    }
  } else {
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "You have to be in a planet scope to buy.\n";
      return;
    }
    snum = g.snum;
    pnum = g.pnum;
    if (Governor && stars[snum].governor[Playernum - 1] != Governor) {
      g.out << "You are not authorized in this system.\n";
      return;
    }
    p = getplanet(snum, pnum);

    if (p.slaved_to && p.slaved_to != Playernum) {
      sprintf(buf, "This planet is enslaved to player %d.\n", p.slaved_to);
      notify(Playernum, Governor, buf);
      return;
    }
    /* check to see if there is an undamaged gov center or space port here */
    Shiplist shiplist(p.ships);
    bool ok = false;
    for (auto s : shiplist) {
      if (s.alive && (s.owner == Playernum) && !s.damage &&
          Shipdata[s.type][ABIL_PORT]) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      g.out << "You don't have an undamaged space port or "
               "government center here.\n";
      return;
    }

    lot = std::stoi(argv[1]);
    money_t bid0 = std::stoi(argv[2]);
    if ((lot <= 0) || lot > g.db.Numcommods()) {
      g.out << "Illegal lot number.\n";
      return;
    }
    auto c = getcommod(lot);
    if (!c.owner) {
      g.out << "No such lot for sale.\n";
      return;
    }
    if (c.owner == g.player &&
        (c.star_from != g.snum || c.planet_from != g.pnum)) {
      g.out << "You can only set a minimum price for your "
               "lot from the location it was sold.\n";
      return;
    }
    money_t minbid = (int)((double)c.bid * (1.0 + UP_BID));
    if (bid0 < minbid) {
      sprintf(buf, "You have to bid more than %ld.\n", minbid);
      notify(Playernum, Governor, buf);
      return;
    }
    auto &race = races[Playernum - 1];
    if (race.Guest) {
      g.out << "Guest races cannot bid.\n";
      return;
    }
    if (bid0 > race.governor[Governor].money) {
      g.out << "Sorry, no buying on credit allowed.\n";
      return;
    }
    /* notify the previous bidder that he was just outbidded */
    if (c.bidder) {
      sprintf(buf,
              "The bid on lot #%d (%lu %s) has been upped to %ld by %s [%d].\n",
              lot, c.amount, commod_name[c.type], bid0, race.name, Playernum);
      notify(c.bidder, c.bidder_gov, buf);
    }
    c.bid = bid0;
    c.bidder = Playernum;
    c.bidder_gov = Governor;
    c.star_to = snum;
    c.planet_to = pnum;
    auto [shipping, dist] = shipping_cost(c.star_to, c.star_from, c.bid);

    sprintf(
        buf,
        "There will be an additional %ld charged to you for shipping costs.\n",
        shipping);
    notify(Playernum, Governor, buf);
    putcommod(c, lot);
    g.out << "Bid accepted.\n";
  }
}
}  // namespace GB::commands
