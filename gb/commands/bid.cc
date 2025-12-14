// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

namespace GB::commands {
void bid(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (argv.size() == 1) {
    /* list all market blocks for sale */
    g.out << "+++ Galactic Bloodshed Commodities Market +++\n\n";

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    // Configure column alignments and widths
    table.column(0).format().width(4).font_align(tabulate::FontAlign::right);   // Lot
    table.column(1).format().width(1).font_align(tabulate::FontAlign::center);  // Deliver marker
    table.column(2).format().width(5).font_align(tabulate::FontAlign::right);   // Stock
    table.column(3).format().width(10).font_align(tabulate::FontAlign::left);   // Type
    table.column(4).format().width(7).font_align(tabulate::FontAlign::right);   // Owner
    table.column(5).format().width(8).font_align(tabulate::FontAlign::right);   // Bidder
    table.column(6).format().width(8).font_align(tabulate::FontAlign::right);   // Amount
    table.column(7).format().width(10).font_align(tabulate::FontAlign::right);  // Cost/Unit
    table.column(8).format().width(8).font_align(tabulate::FontAlign::right);   // Ship
    table.column(9).format().width(10).font_align(tabulate::FontAlign::left);   // Dest

    // Add header row
    table.add_row({"Lot", "", "Stock", "Type", "Owner", "Bidder", "Amount",
                   "Cost/Unit", "Ship", "Dest"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    // Add data rows
    for (const auto* c : CommodList(g.entity_manager)) {
      auto rate = (double)c->bid / (double)c->amount;
      const auto* star_to = g.entity_manager.peek_star(c->star_to);
      std::string player_details =
          (c->bidder == Playernum)
              ? std::format("{:4.4s}/{:<4.4s}", star_to->get_name(),
                            star_to->get_planet_name(c->planet_to))
              : "";

      auto [cost, dist] =
          shipping_cost(g.entity_manager, c->star_from, g.snum, c->bid);
      
      table.add_row({std::format("{}", c->id),
                     c->deliver ? "*" : "",
                     std::format("{}", c->amount),
                     std::format("{}", c->type),
                     std::format("{}", c->owner),
                     std::format("{}", c->bidder),
                     std::format("{}", c->bid),
                     std::format("{:.2f}", rate),
                     std::format("{}", cost),
                     player_details});
    }

    g.out << table << "\n";
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

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    // Configure column alignments and widths
    table.column(0).format().width(4).font_align(tabulate::FontAlign::right);   // Lot
    table.column(1).format().width(1).font_align(tabulate::FontAlign::center);  // Deliver marker
    table.column(2).format().width(5).font_align(tabulate::FontAlign::right);   // Stock
    table.column(3).format().width(10).font_align(tabulate::FontAlign::left);   // Type
    table.column(4).format().width(7).font_align(tabulate::FontAlign::right);   // Owner
    table.column(5).format().width(8).font_align(tabulate::FontAlign::right);   // Bidder
    table.column(6).format().width(8).font_align(tabulate::FontAlign::right);   // Amount
    table.column(7).format().width(10).font_align(tabulate::FontAlign::right);  // Cost/Unit
    table.column(8).format().width(8).font_align(tabulate::FontAlign::right);   // Ship
    table.column(9).format().width(10).font_align(tabulate::FontAlign::left);   // Dest

    // Add header row
    table.add_row({"Lot", "", "Stock", "Type", "Owner", "Bidder", "Amount",
                   "Cost/Unit", "Ship", "Dest"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    // Add data rows
    for (const auto* c : CommodList(g.entity_manager)) {
      if (c->type != item) continue;
      auto rate = (double)c->bid / (double)c->amount;
      const auto* star_to = g.entity_manager.peek_star(c->star_to);
      std::string player_details =
          (c->bidder == Playernum)
              ? std::format("{:4.4s}/{:<4.4s}", star_to->get_name(),
                            star_to->get_planet_name(c->star_to))
              : "";
      auto [cost, dist] =
          shipping_cost(g.entity_manager, c->star_from, g.snum, c->bid);
      
      table.add_row({std::format("{}", c->id),
                     c->deliver ? "*" : "",
                     std::format("{}", c->amount),
                     std::format("{}", c->type),
                     std::format("{}", c->owner),
                     std::format("{}", c->bidder),
                     std::format("{}", c->bid),
                     std::format("{:.2f}", rate),
                     std::format("{}", cost),
                     player_details});
    }

    g.out << table << "\n";
  } else {
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "You have to be in a planet scope to buy.\n";
      return;
    }
    auto snum = g.snum;
    auto pnum = g.pnum;
    const auto* star = g.entity_manager.peek_star(snum);
    if (Governor && star->governor(Playernum - 1) != Governor) {
      g.out << "You are not authorized in this system.\n";
      return;
    }
    const auto* p = g.entity_manager.peek_planet(snum, pnum);

    if (p->slaved_to() && p->slaved_to() != Playernum) {
      g.out << std::format("This planet is enslaved to player {}.\n",
                           p->slaved_to());
      return;
    }
    /* check to see if there is an undamaged gov center or space port here */
    const ShipList kShiplist(g.entity_manager, p->ships());
    bool ok = false;
    for (const Ship* s : kShiplist) {
      if (s->alive() && (s->owner() == Playernum) && !s->damage() &&
          Shipdata[s->type()][ABIL_PORT]) {
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
    if ((lot <= 0) || lot > g.entity_manager.num_commods()) {
      g.out << "Illegal lot number.\n";
      return;
    }

    // First peek to validate the lot
    const auto* c_peek = g.entity_manager.peek_commod(lot);
    if (!c_peek || !c_peek->owner) {
      g.out << "No such lot for sale.\n";
      return;
    }
    if (c_peek->owner == g.player &&
        (c_peek->star_from != g.snum || c_peek->planet_from != g.pnum)) {
      g.out << "You can only set a minimum price for your "
               "lot from the location it was sold.\n";
      return;
    }
    money_t minbid = (int)((double)c_peek->bid * (1.0 + UP_BID));
    if (bid0 < minbid) {
      g.out << std::format("You have to bid more than {}.\n", minbid);
      return;
    }
    if (g.race->Guest) {
      g.out << "Guest races cannot bid.\n";
      return;
    }
    // Need to check money via g.race->governor
    if (bid0 > g.race->governor[Governor].money) {
      g.out << "Sorry, no buying on credit allowed.\n";
      return;
    }

    auto commod_handle = g.entity_manager.get_commod(lot);
    auto& c = *commod_handle;

    /* notify the previous bidder that he was just outbidded */
    if (c.bidder) {
      std::string bid_message = std::format(
          "The bid on lot #{} ({} {}) has been upped to {} by {} [{}].\n", lot,
          c.amount, c.type, bid0, g.race->name, Playernum);
      notify(c.bidder, c.bidder_gov, bid_message);
    }
    c.bid = bid0;
    c.bidder = Playernum;
    c.bidder_gov = Governor;
    c.star_to = snum;
    c.planet_to = pnum;
    auto [shipping, dist] =
        shipping_cost(g.entity_manager, c.star_to, c.star_from, c.bid);

    g.out << std::format(
        "There will be an additional {} charged to you for shipping costs.\n",
        shipping);
    g.out << "Bid accepted.\n";
  }
}
}  // namespace GB::commands
