// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void sell(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  ap_t APcount = 20;

  if (!MARKET) return;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to be in a planet scope to sell.\n";
    return;
  }
  auto snum = g.snum;
  auto pnum = g.pnum;
  if (argv.size() < 3) {
    g.out << "Syntax: sell <commodity> <amount>\n";
    return;
  }
  if (Governor && stars[snum].governor(Playernum - 1) != Governor) {
    g.out << "You are not authorized in this system.\n";
    return;
  }
  auto &race = races[Playernum - 1];
  if (race.Guest) {
    g.out << "Guest races can't sell anything.\n";
    return;
  }
  /* get information on sale */
  auto commod = argv[1][0];
  auto amount = std::stoi(argv[2]);
  if (amount <= 0) {
    g.out << "Try using positive values.\n";
    return;
  }
  APcount = MIN(APcount, amount);
  if (!enufAP(Playernum, Governor, stars[snum].AP(Playernum - 1), APcount))
    return;
  auto p = getplanet(snum, pnum);

  if (p.slaved_to && p.slaved_to != Playernum) {
    g.out << std::format("This planet is enslaved to player {}.\n",
                         p.slaved_to);
    return;
  }
  /* check to see if there is an undamage gov center or space port here */
  bool ok = false;
  Shiplist shiplist(p.ships);
  for (auto s : shiplist) {
    if (s.alive && (s.owner == Playernum) && !s.damage &&
        Shipdata[s.type][ABIL_PORT]) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    g.out << "You don't have an undamaged space port or government center "
             "here.\n";
    return;
  }
  CommodType item;
  switch (commod) {
    case 'r':
      if (!p.info[Playernum - 1].resource) {
        g.out << "You don't have any resources here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].resource);
      p.info[Playernum - 1].resource -= amount;
      item = CommodType::RESOURCE;
      break;
    case 'd':
      if (!p.info[Playernum - 1].destruct) {
        g.out << "You don't have any destruct here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].destruct);
      p.info[Playernum - 1].destruct -= amount;
      item = CommodType::DESTRUCT;
      break;
    case 'f':
      if (!p.info[Playernum - 1].fuel) {
        g.out << "You don't have any fuel here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].fuel);
      p.info[Playernum - 1].fuel -= amount;
      item = CommodType::FUEL;
      break;
    case 'x':
      if (!p.info[Playernum - 1].crystals) {
        g.out << "You don't have any crystals here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].crystals);
      p.info[Playernum - 1].crystals -= amount;
      item = CommodType::CRYSTAL;
      break;
    default:
      g.out << "Permitted commodities are r, d, f, and x.\n";
      return;
  }

  Commod c;
  c.owner = Playernum;
  c.governor = Governor;
  c.type = item;
  c.amount = amount;
  c.deliver = false;
  c.bid = 0;
  c.bidder = 0;
  c.star_from = snum;
  c.planet_from = pnum;

  int commodno;
  while ((commodno = getdeadcommod()) == 0);

  if (commodno == -1) commodno = g.db.Numcommods() + 1;
  g.out << std::format("Lot #{} - {} units of {}.\n", commodno, amount, item);
  std::string buf =
      std::format("Lot #{} - {} units of {} for sale by {} [{}].\n", commodno,
                  amount, item, races[Playernum - 1].name, Playernum);
  post(buf, NewsType::TRANSFER);
  for (player_t i = 1; i <= Num_races; i++) notify_race(i, buf);
  putcommod(c, commodno);
  putplanet(p, stars[snum], pnum);
  deductAPs(g, APcount, snum);
}
}  // namespace GB::commands
