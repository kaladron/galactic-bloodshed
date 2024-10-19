// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void bid(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (argv.size() == 1) {
    /* list all market blocks for sale */
    notify(Playernum, Governor,
           "+++ Galactic Bloodshed Commodities Market +++\n\n");
    notify(Playernum, Governor,
           "  Lot Stock      Type  Owner  Bidder  Amount "
           "Cost/Unit    Ship  Dest\n");
    for (auto i = 1; i <= g.db.Numcommods(); i++) {
      auto c = getcommod(i);
      if (c.owner && c.amount) {
        auto rate = (double)c.bid / (double)c.amount;
        std::string player_details =
            (c.bidder == Playernum)
                ? std::format("{:4.4s}/{:<4.4s}", stars[c.star_to].get_name(),
                              stars[c.star_to].get_planet_name(c.planet_to))
                : "";

        auto [cost, dist] = shipping_cost(c.star_from, g.snum, c.bid);
        g.out << std::format(" {:4}{}{:5}{:10}{:7}{:8}{:8}{:10.2}{:8} {:10}\n",
                             i, c.deliver ? '*' : ' ', c.amount, c.type,
                             c.owner, c.bidder, c.bid, rate, cost,
                             player_details);
      }
    }
  } else if (argv.size() == 2) {
    /* list all market blocks for sale of the requested type */
    auto commod = argv[1][0];
    CommodType item;
    switch (commod) {
      case 'r':
        item = CommodType::RESOURCE;
        break;
      case 'd':
        item = CommodType::DESTRUCT;
        break;
      case 'f':
        item = CommodType::FUEL;
        break;
      case 'x':
        item = CommodType::CRYSTAL;
        break;
      default:
        g.out << "No such type of commodity.\n";
        return;
    }
    g.out << "+++ Galactic Bloodshed Commodities Market +++\n\n";
    g.out << "  Lot Stock      Type  Owner  Bidder  Amount "
             "Cost/Unit    Ship  Dest\n";
    for (auto i = 1; i <= g.db.Numcommods(); i++) {
      auto c = getcommod(i);
      if (c.owner && c.amount && (c.type == item)) {
        auto rate = (double)c.bid / (double)c.amount;
        std::string player_details =
            (c.bidder == Playernum)
                ? std::format("{:4.4s}/{:<4.4s}", stars[c.star_to].get_name(),
                              stars[c.star_to].get_planet_name(c.planet_to))
                : "";
        auto [cost, dist] = shipping_cost(c.star_from, g.snum, c.bid);
        g.out << std::format(" {:4}{}{:5}{:10}{:7}{:8}{:8}{:10.2}{:8} {:10}\n",
                             i, c.deliver ? '*' : ' ', c.amount, c.type,
                             c.owner, c.bidder, c.bid, rate, cost,
                             player_details);
      }
    }
  } else {
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "You have to be in a planet scope to buy.\n";
      return;
    }
    auto snum = g.snum;
    auto pnum = g.pnum;
    if (Governor && stars[snum].governor(Playernum - 1) != Governor) {
      g.out << "You are not authorized in this system.\n";
      return;
    }
    auto p = getplanet(snum, pnum);

    if (p.slaved_to && p.slaved_to != Playernum) {
      g.out << std::format("This planet is enslaved to player {}.\n",
                           p.slaved_to);
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

    auto lot = std::stoi(argv[1]);
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
      g.out << std::format("You have to bid more than {}.\n", minbid);
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
      std::string bid_message = std::format(
          "The bid on lot #{} ({} {}) has been upped to {} by {} [{}].\n", lot,
          c.amount, c.type, bid0, race.name, Playernum);
      notify(c.bidder, c.bidder_gov, bid_message);
    }
    c.bid = bid0;
    c.bidder = Playernum;
    c.bidder_gov = Governor;
    c.star_to = snum;
    c.planet_to = pnum;
    auto [shipping, dist] = shipping_cost(c.star_to, c.star_from, c.bid);

    g.out << std::format(
        "There will be an additional {} charged to you for shipping costs.\n",
        shipping);
    putcommod(c, lot);
    g.out << "Bid accepted.\n";
  }
}
}  // namespace GB::commands
