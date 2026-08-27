// SPDX-License-Identifier: Apache-2.0

/// \file sell.cc
/// \brief Sell commodities on the planetary market.

module;

import gblib;
import notification;
import session;
import std;
import scnlib;

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
  auto parsed_amount = scn::scan<int>(argv[2], "{}");
  if (!parsed_amount || parsed_amount->value() <= 0) {
    g.out << "Try using positive values.\n";
    return false;
  }
  int amount = parsed_amount->value();

  const auto& p_peek = *g.entity_manager.peek_planet(snum, pnum);

  if (p_peek.slaved_to() != 0 && p_peek.slaved_to() != Playernum) {
    g.out << std::format("This planet is enslaved to player {}.\n",
                         p_peek.slaved_to());
    return false;
  }

  /* check to see if there is an undamaged gov center or space port here */
  bool ok = false;
  for (const Ship* s : ShipList::readonly(g)) {
    if (s->alive() && (s->owner() == Playernum) && !s->damage() &&
        Shipdata[s->type()][ABIL_PORT]) {
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
      if (!p_peek.info(Playernum).resource) {
        g.out << "You don't have any resources here to sell!\n";
        return false;
      }
      amount = std::min<int>(amount, p_peek.info(Playernum).resource);
      item = CommodType::RESOURCE;
      break;
    case 'd':
      if (!p_peek.info(Playernum).destruct) {
        g.out << "You don't have any destruct here to sell!\n";
        return false;
      }
      amount = std::min<int>(amount, p_peek.info(Playernum).destruct);
      item = CommodType::DESTRUCT;
      break;
    case 'f':
      if (!p_peek.info(Playernum).fuel) {
        g.out << "You don't have any fuel here to sell!\n";
        return false;
      }
      amount = std::min<int>(amount, p_peek.info(Playernum).fuel);
      item = CommodType::FUEL;
      break;
    case 'x':
      if (!p_peek.info(Playernum).crystals) {
        g.out << "You don't have any crystals here to sell!\n";
        return false;
      }
      amount = std::min<int>(amount, p_peek.info(Playernum).crystals);
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

  g.entity_manager.mutate_planet(snum, pnum, [&](Planet& p) {
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
  });

  int commodno = g.entity_manager.next_available_commod_id();
  if (commodno == -1) commodno = g.entity_manager.max_commod_id() + 1;
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
