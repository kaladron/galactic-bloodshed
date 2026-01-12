// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import notification;
import session; // For SessionRegistry full definition
import std;

module commands;

namespace GB::commands {
void sell(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  ap_t APcount = 20;

  if (!MARKET) return;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to be in a planet scope to sell.\n";
    return;
  }
  auto snum = g.snum();
  auto pnum = g.pnum();
  if (argv.size() < 3) {
    g.out << "Syntax: sell <commodity> <amount>\n";
    return;
  }
  const auto* star = g.entity_manager.peek_star(snum);
  if (Governor != 0 && star->governor(Playernum) != Governor) {
    g.out << "You are not authorized in this system.\n";
    return;
  }
  if (g.race->Guest) {
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
  if (!enufAP(g.entity_manager, Playernum, Governor, star->AP(Playernum),
              APcount))
    return;

  auto planet_handle = g.entity_manager.get_planet(snum, pnum);
  auto& p = *planet_handle;

  if (p.slaved_to() != 0 && p.slaved_to() != Playernum) {
    g.out << std::format("This planet is enslaved to player {}.\n",
                         p.slaved_to());
    return;
  }
  /* check to see if there is an undamage gov center or space port here */
  bool ok = false;
  ShipList ships(g.entity_manager, p.ships());
  for (auto ship_handle : ships) {
    const Ship& s = ship_handle.peek();  // Read-only access
    if (s.alive() && (s.owner() == Playernum) && !s.damage() &&
        Shipdata[s.type()][ABIL_PORT]) {
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
      if (!p.info(Playernum).resource) {
        g.out << "You don't have any resources here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info(Playernum).resource);
      p.info(Playernum).resource -= amount;
      item = CommodType::RESOURCE;
      break;
    case 'd':
      if (!p.info(Playernum).destruct) {
        g.out << "You don't have any destruct here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info(Playernum).destruct);
      p.info(Playernum).destruct -= amount;
      item = CommodType::DESTRUCT;
      break;
    case 'f':
      if (!p.info(Playernum).fuel) {
        g.out << "You don't have any fuel here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info(Playernum).fuel);
      p.info(Playernum).fuel -= amount;
      item = CommodType::FUEL;
      break;
    case 'x':
      if (!p.info(Playernum).crystals) {
        g.out << "You don't have any crystals here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info(Playernum).crystals);
      p.info(Playernum).crystals -= amount;
      item = CommodType::CRYSTAL;
      break;
    default:
      g.out << "Permitted commodities are r, d, f, and x.\n";
      return;
  }

  int commodno = g.entity_manager.next_available_commod_id();

  if (commodno == -1) commodno = g.entity_manager.num_commods() + 1;
  g.out << std::format("Lot #{} - {} units of {}.\n", commodno, amount, item);
  std::string buf =
      std::format("Lot #{} - {} units of {} for sale by {} [{}].\n", commodno,
                  amount, item, g.race->name, Playernum);
  post(g.entity_manager, buf, NewsType::TRANSFER);
  for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
    g.session_registry.notify_race(i, buf);
  }

  Commod c{};
  c.owner = Playernum;
  c.governor = Governor;
  c.type = item;
  c.amount = amount;
  c.deliver = false;
  c.bid = 0;
  c.bidder = 0;
  c.star_from = snum;
  c.planet_from = pnum;
  c.star_to = 0;
  c.planet_to = 0;

  auto commod_handle = g.entity_manager.create_commod(c);

  deductAPs(g, APcount, snum);
}
}  // namespace GB::commands
