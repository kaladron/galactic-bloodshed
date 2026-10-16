// SPDX-License-Identifier: Apache-2.0

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {
namespace {

std::optional<CommodType> parse_commod_type(std::string_view arg) {
  if (arg.empty()) return std::nullopt;
  switch (arg[0]) {
    case 'r':
      return CommodType::RESOURCE;
    case 'd':
      return CommodType::DESTRUCT;
    case 'f':
      return CommodType::FUEL;
    case 'x':
      return CommodType::CRYSTAL;
    default:
      return std::nullopt;
  }
}

void present_commodities(GameObj& g, std::optional<CommodType> filter) {
  GB::presentation::CommodityMarketViewModel vm{.filter = filter};

  for (const Commod& c : CommodList::readonly(g.entity_manager)) {
    if (filter && c.type() != *filter) continue;

    std::string destination;
    if (c.has_bid() && c.bidder() == g.player()) {
      const auto* star_to = g.entity_manager.peek_star(*c.star_to());
      destination = std::format("{:4.4s}/{:<4.4s}", star_to->get_name(),
                                star_to->get_planet_name(*c.planet_to()));
    }

    money_t ship_cost = 0;
    if (g.level() == ScopeLevel::LEVEL_STAR ||
        g.level() == ScopeLevel::LEVEL_PLAN) {
      std::tie(ship_cost, std::ignore) =
          shipping_cost(g.entity_manager, c.star_from(), g.snum(), c.bid());
    }

    vm.rows.push_back(GB::presentation::CommodityLotRow{
        .lot = c.id(),
        .deliver = c.deliver(),
        .amount = c.amount(),
        .type = c.type(),
        .owner = c.owner(),
        .bidder = c.bidder().value_or(player_t{0}),
        .bid = c.bid(),
        .cost_per_unit = c.cost_per_unit(),
        .shipping_cost = ship_cost,
        .destination = std::move(destination),
    });
  }

  g.present(vm);
}

bool place_bid(const command_t& argv, GameObj& g) {
  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to be in a planet scope to buy.\n";
    return false;
  }
  auto snum = g.snum();
  auto pnum = g.pnum();
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star->control(g.player(), g.governor())) {
    g.out << "You are not authorized in this system.\n";
    return false;
  }
  const auto* p = g.entity_manager.peek_planet(snum, pnum);

  if (p->is_enslaved_to_foreign(g.player())) {
    g.out << std::format("This planet is enslaved to player {}.\n",
                         *p->slaved_to());
    return false;
  }
  /* check to see if there is an undamaged gov center or space port here */
  bool has_starport = false;
  for (const Ship& s : ShipList::readonly(g)) {
    if (s.alive() && (s.owner() == g.player()) && !s.damage() &&
        s.is_starport()) {
      has_starport = true;
      break;
    }
  }
  if (!has_starport) {
    g.out << "You don't have an undamaged space port or "
             "government center here.\n";
    return false;
  }

  auto parsed_lot = scn::scan<int>(argv[1], "{}");
  auto parsed_bid = scn::scan<money_t>(argv[2], "{}");
  if (!parsed_lot || !parsed_bid) {
    g.out << "Illegal lot number.\n";
    return false;
  }
  int lot = parsed_lot->value();
  money_t bid_amount = parsed_bid->value();
  if ((lot <= 0) || lot > g.entity_manager.max_commod_id()) {
    g.out << "Illegal lot number.\n";
    return false;
  }

  const Commod* c_peek = nullptr;
  try {
    c_peek = g.entity_manager.peek_commod(lot);
  } catch (const EntityNotFoundError&) {
    g.out << "No such lot for sale.\n";
    return false;
  }
  if (c_peek->owner() == g.player() && !c_peek->is_origin(g.snum(), g.pnum())) {
    g.out << "You can only set a minimum price for your "
             "lot from the location it was sold.\n";
    return false;
  }
  money_t minbid = c_peek->min_next_bid();
  if (bid_amount <= 0 || bid_amount < minbid) {
    g.out << std::format("You have to bid more than {}.\n", minbid);
    return false;
  }
  if (g.race->Guest) {
    g.out << "Guest races cannot bid.\n";
    return false;
  }
  if (bid_amount > g.current_governor().money) {
    g.out << "Sorry, no buying on credit allowed.\n";
    return false;
  }

  money_t shipping = 0;
  g.entity_manager.mutate_commod(lot, [&](Commod& c) {
    if (c.bidder()) {
      std::string bid_message = GB::presentation::render_outbid_telegram(
          lot, c.amount(), c.type(), bid_amount, g.race->name, g.player());
      warn_player(g.session_registry, g.entity_manager, *c.bidder(),
                  c.bidder_gov(), bid_message);
    }
    c.place_bid(g.player(), g.governor(), bid_amount, snum, pnum);
    auto [ship_cost, dist] =
        shipping_cost(g.entity_manager, *c.star_to(), c.star_from(), c.bid());
    shipping = ship_cost;
  });

  g.present(GB::presentation::BidPlacedResult{
      .lot = lot,
      .bid = bid_amount,
      .shipping_cost = shipping,
  });
  return true;
}
}  // namespace

bool bid(const command_t& argv, GameObj& g) {
  if (argv.size() == 1) {
    present_commodities(g, std::nullopt);
    return true;
  }
  if (argv.size() == 2) {
    auto item = parse_commod_type(argv[1]);
    if (!item) {
      g.out << "No such type of commodity.\n";
      return false;
    }
    present_commodities(g, item);
    return true;
  }
  return place_bid(argv, g);
}

const CommandDescriptor bid_cmd{
    .name = "bid",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "bid [<type> | <lot> <bid>]",
    .description = "View market lots or place bids on commodities",
    .handler = &bid,
};

}  // namespace GB::commands
