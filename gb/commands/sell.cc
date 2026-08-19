// SPDX-License-Identifier: Apache-2.0

/// \file sell.cc
/// \brief Sell commodities on the planetary market.

module;

import gblib;
import notification;
import session;
import std;

module commands;

namespace GB::commands {

bool sell(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();

  if (!MARKET) return false;

  auto snum = g.snum();
  auto pnum = g.pnum();

  /* get information on sale */
  auto commod = argv[1][0];
  int amount = 0;
  try {
    amount = std::stoi(argv[2]);
  } catch (...) {
    g.out << "Try using positive values.\n";
    return false;
  }
  if (amount <= 0) {
    g.out << "Try using positive values.\n";
    return false;
  }
  auto planet_handle = g.entity_manager.get_planet(snum, pnum);
  auto& p = *planet_handle;

  if (p.slaved_to() != 0 && p.slaved_to() != Playernum) {
    g.out << std::format("This planet is enslaved to player {}.\n",
                         p.slaved_to());
    return false;
  }

  /* check to see if there is an undamage gov center or space port here */
  bool ok = false;
  ShipList ships(g.entity_manager, p.ships());
  for (auto ship_handle : ships) {
    const Ship& s = ship_handle.peek();
    if (s.alive() && (s.owner() == Playernum) && !s.damage() &&
        Shipdata[s.type()][ABIL_PORT]) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    g.out << "You don't have an undamaged space port or government center "
             "here.\n";
    return false;
  }
  CommodType item;
  switch (commod) {
    case 'r':
      if (!p.info(Playernum).resource) {
        g.out << "You don't have any resources here to sell!\n";
        return false;
      }
      amount = MIN(amount, p.info(Playernum).resource);
      item = CommodType::RESOURCE;
      break;
    case 'd':
      if (!p.info(Playernum).destruct) {
        g.out << "You don't have any destruct here to sell!\n";
        return false;
      }
      amount = MIN(amount, p.info(Playernum).destruct);
      item = CommodType::DESTRUCT;
      break;
    case 'f':
      if (!p.info(Playernum).fuel) {
        g.out << "You don't have any fuel here to sell!\n";
        return false;
      }
      amount = MIN(amount, p.info(Playernum).fuel);
      item = CommodType::FUEL;
      break;
    case 'x':
      if (!p.info(Playernum).crystals) {
        g.out << "You don't have any crystals here to sell!\n";
        return false;
      }
      amount = MIN(amount, p.info(Playernum).crystals);
      item = CommodType::CRYSTAL;
      break;
    default:
      g.out << "Permitted commodities are r, d, f, and x.\n";
      return false;
  }

  ap_t APcount = MIN(20, amount);
  if (!g.deduct_ap(snum, APcount)) {
    g.out << std::format("You don't have {} action points there.\n", APcount);
    return false;
  }

  switch (item) {
    case CommodType::RESOURCE:
      p.info(Playernum).resource -= amount;
      break;
    case CommodType::DESTRUCT:
      p.info(Playernum).destruct -= amount;
      break;
    case CommodType::FUEL:
      p.info(Playernum).fuel -= amount;
      break;
    case CommodType::CRYSTAL:
      p.info(Playernum).crystals -= amount;
      break;
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
  return true;
}

const CommandDescriptor sell_cmd{
    .name = "sell",
    .roles =
        {
            .no_guests = true,
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "sell <r|d|f|x> <amount>",
    .description = "Sell commodities on the market",
    .handler = &sell,
};

}  // namespace GB::commands
