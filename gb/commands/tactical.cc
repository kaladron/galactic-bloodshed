// SPDX-License-Identifier: Apache-2.0

/// \file tactical.cc
/// \brief Tactical combat reporting command
///
/// Tactical reports show firing solutions, hit probabilities, and target
/// information for combat scenarios. Unlike other reports, tactical requires
/// collecting all ships and planets in range to calculate distance-based
/// firing solutions.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

using GB::presentation::TacticalPlanetHeader;
using GB::presentation::TacticalPlanetTargetRow;
using GB::presentation::TacticalReportEntry;
using GB::presentation::TacticalShipHeader;
using GB::presentation::TacticalShipTargetRow;
using GB::presentation::TacticalTargetEntry;
using GB::presentation::TacticalViewModel;

struct TacticalParams {
  double tech = 0.0;
  double weapon_range = 0.0;
  damage_t damage = 0;
  bool fev = false;
  speed_t fspeed = 0;
  guntype_t caliber = guntype_t::MEDIUM;
  bool laser_focused = false;
};

struct TacticalContext {
  std::string shiplist;
  bool enemies_only = false;
  std::optional<player_t>
      filter_player;  // Filter by player number (nullopt = no filter)
};

class TacticalItem {
protected:
  const UniverseCoordinates coords_;

public:
  explicit TacticalItem(UniverseCoordinates coords) : coords_(coords) {}
  virtual ~TacticalItem() = default;

  // Non-copyable and non-movable (polymorphic base class with unique ownership)
  TacticalItem(const TacticalItem&) = delete;
  TacticalItem& operator=(const TacticalItem&) = delete;
  TacticalItem(TacticalItem&&) = delete;
  TacticalItem& operator=(TacticalItem&&) = delete;

  UniverseCoordinates coordinates() const {
    return coords_;
  }

  virtual std::optional<shipnum_t> ship_number() const {
    return std::nullopt;
  }

  virtual void populate_header(TacticalReportEntry& entry, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const = 0;

  virtual std::optional<TacticalTargetEntry>
  build_target_entry(GameObj& g, TacticalContext& ctx, const Race& race,
                     double dist, const TacticalParams& firer) const = 0;

  virtual TacticalParams get_tactical_params(const Race&) const = 0;

  virtual bool should_report_tactical(player_t player_num,
                                      governor_t governor) const = 0;

  TacticalReportEntry
  build_report_entry(GameObj& g, TacticalContext& ctx, const Race& race,
                     const std::vector<std::unique_ptr<TacticalItem>>& items,
                     const TacticalParams& params) const;
};

class ShipTacticalItem : public TacticalItem {
  const Ship* ship_;

public:
  explicit ShipTacticalItem(const Ship* ship)
      : TacticalItem(ship->coordinates()), ship_(ship) {}

  std::optional<shipnum_t> ship_number() const override {
    return ship_->number();
  }

  void populate_header(TacticalReportEntry& entry, GameObj& g,
                       player_t player_num,
                       const TacticalParams& params) const override;

  std::optional<TacticalTargetEntry>
  build_target_entry(GameObj& g, TacticalContext& ctx, const Race& race,
                     double dist, const TacticalParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report_tactical(player_t player_num,
                              governor_t governor) const override;
};

class PlanetTacticalItem : public TacticalItem {
  const Planet* planet_;

public:
  PlanetTacticalItem(const Planet* planet, UniverseCoordinates coords)
      : TacticalItem(coords), planet_(planet) {}

  void populate_header(TacticalReportEntry& entry, GameObj& g,
                       player_t player_num,
                       const TacticalParams& params) const override;

  std::optional<TacticalTargetEntry>
  build_target_entry(GameObj& g, TacticalContext& ctx, const Race& race,
                     double dist, const TacticalParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report_tactical(player_t player_num,
                              governor_t governor) const override;
};

/* Add a ship to the tactical item list if not already present */
void add_tactical_ship(std::vector<std::unique_ptr<TacticalItem>>& items,
                       const Ship* ship) {
  for (const auto& existing : items) {
    if (existing->ship_number() == ship->number()) {
      return;
    }
  }
  items.push_back(std::make_unique<ShipTacticalItem>(ship));
}

void plan_get_tactical_items(GameObj& g,
                             std::vector<std::unique_ptr<TacticalItem>>& items,
                             player_t player_num, starnum_t snum,
                             planetnum_t pnum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  const auto* planet = g.entity_manager.peek_planet(snum, pnum);
  if (!planet) return;

  // Add planet to tactical list
  items.push_back(std::make_unique<PlanetTacticalItem>(
      planet, planet->absolute_coordinates(*star)));

  if (planet->info(player_num).explored) {
    for (const Ship& ship :
         ShipList::readonly_on_planet(g.entity_manager, snum, pnum)) {
      add_tactical_ship(items, &ship);
    }
  }
}

void star_get_tactical_items(GameObj& g,
                             std::vector<std::unique_ptr<TacticalItem>>& items,
                             player_t player_num, starnum_t snum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (star->is_explored_by(player_num)) {
    for (const Ship& ship :
         ShipList::readonly_in_star(g.entity_manager, snum)) {
      add_tactical_ship(items, &ship);
    }

    for (const auto& planet :
         PlanetList::readonly(g.entity_manager, snum, star->numplanets())) {
      plan_get_tactical_items(g, items, player_num, snum,
                              planet.planet_order());
    }
  }
}

TacticalReportEntry TacticalItem::build_report_entry(
    GameObj& g, TacticalContext& ctx, const Race& race,
    const std::vector<std::unique_ptr<TacticalItem>>& items,
    const TacticalParams& params) const {
  TacticalReportEntry entry{};
  populate_header(entry, g, g.player(), params);

  for (const auto& target : items) {
    if (target.get() == this) continue;

    const double dist = coords_.distance_to(target->coordinates());
    if (dist >= params.weapon_range) continue;

    if (auto target_row =
            target->build_target_entry(g, ctx, race, dist, params)) {
      entry.targets.push_back(std::move(*target_row));
    }
  }

  return entry;
}

void ShipTacticalItem::populate_header(TacticalReportEntry& entry, GameObj& g,
                                       player_t,
                                       const TacticalParams& params) const {
  const auto& s = *ship_;
  std::optional<Coordinates> landed_coords{std::nullopt};
  if (s.is_landed()) {
    landed_coords = s.land_coords();
  }

  entry.ship_header = TacticalShipHeader{
      .number = s.number(),
      .type_letter = s.type_letter(),
      .active = s.active(),
      .name = s.name(),
      .tech = s.tech(),
      .battery_summary = s.battery_summary(),
      .armor = s.armor(),
      .size = s.size(),
      .destruct = s.destruct(),
      .fuel = s.fuel(),
      .damage = s.damage(),
      .fspeed = params.fspeed,
      .evade = params.fev,
      .orbits = dispshiploc(g.entity_manager, s),
      .landed_coords = landed_coords,
      .radiation = s.rad(),
  };
}

bool is_valid_ship_target(const Ship& s, const GameObj& g,
                          const TacticalContext& ctx, const Race& race) {
  if (ctx.filter_player.has_value() && s.owner() != *ctx.filter_player) {
    return false;
  }
  if (!ctx.shiplist.empty() && !listed(s.type(), ctx.shiplist)) {
    return false;
  }
  if (s.owner() == g.player() && s.is_authorized_for(g.governor())) {
    return false;
  }
  if (!s.alive() || s.type() == ShipType::OTYPE_CANIST ||
      s.type() == ShipType::OTYPE_GREEN) {
    return false;
  }
  if (ctx.enemies_only && race.is_allied_with(s.owner())) {
    return false;
  }
  return true;
}

std::optional<TacticalTargetEntry>
ShipTacticalItem::build_target_entry(GameObj& g, TacticalContext& ctx,
                                     const Race& race, double dist,
                                     const TacticalParams& firer) const {
  const auto& s = *ship_;
  if (!is_valid_ship_target(s, g, ctx, race)) {
    return std::nullopt;
  }

  // Calculate target ship's evasion and speed (only if moving and active)
  bool tev = false;
  speed_t tspeed = 0;
  if ((s.whatdest() != ScopeLevel::LEVEL_UNIV || s.navigate().on) &&
      !s.docked() && s.active()) {
    tspeed = s.speed();
    tev = s.protect().evade;
  }

  // Calculate combat parameters using firer's data and target's data
  ship_size_t body = s.size();
  auto defense = getdefense(g.entity_manager, s);
  auto [prob, factor] =
      hit_odds(dist, firer.tech, firer.damage, firer.fev, tev, firer.fspeed,
               tspeed, body, firer.caliber, defense);

  if (firer.laser_focused) {
    prob = prob * prob / 100;
  }

  const char relation_indicator = race.is_at_war_with(s.owner())   ? '-'
                                  : race.is_allied_with(s.owner()) ? '+'
                                                                   : ' ';
  std::optional<Coordinates> landed_coords{std::nullopt};
  if (s.is_landed()) {
    landed_coords = s.land_coords();
  }

  return TacticalTargetEntry{
      .ship =
          TacticalShipTargetRow{
              .number = s.number(),
              .relation_indicator = relation_indicator,
              .owner = s.owner(),
              .governor = s.governor(),
              .type_letter = s.type_letter(),
              .name = s.name(),
              .distance = dist,
              .range_50_pct = factor,
              .size = body,
              .speed = tspeed,
              .evade = tev,
              .hit_prob = prob,
              .damage = s.damage(),
              .landed_coords = landed_coords,
              .active = s.active(),
          },
  };
}

TacticalParams ShipTacticalItem::get_tactical_params(const Race&) const {
  const auto& s = *ship_;
  TacticalParams params{
      .tech = s.tech(),
      .weapon_range = s.gun_range(),
      .damage = s.damage(),
      .caliber = s.current_caliber(),
      .laser_focused = (s.is_laser_on() && s.focus()),
  };

  if ((s.whatdest() != ScopeLevel::LEVEL_UNIV || s.navigate().on) &&
      !s.docked() && s.active()) {
    params.fspeed = s.speed();
    params.fev = s.protect().evade;
  }

  return params;
}

bool ShipTacticalItem::should_report_tactical(player_t player_num,
                                              governor_t governor) const {
  const auto& s = *ship_;
  return s.alive() && s.owner() == player_num &&
         s.is_authorized_for(governor) && s.has_sight();
}

void PlanetTacticalItem::populate_header(TacticalReportEntry& entry, GameObj& g,
                                         player_t player_num,
                                         const TacticalParams& params) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id());

  entry.planet_header = TacticalPlanetHeader{
      .planet_name = star ? star->get_planet_name(p.planet_order()) : "Unknown",
      .tech = params.tech,
      .guns = p.info(player_num).guns,
      .destruct = p.info(player_num).destruct,
      .fuel = p.info(player_num).fuel,
  };
}

std::optional<TacticalTargetEntry>
PlanetTacticalItem::build_target_entry(GameObj& g, TacticalContext&,
                                       const Race&, double dist,
                                       const TacticalParams&) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id());

  return TacticalTargetEntry{
      .planet =
          TacticalPlanetTargetRow{
              .planet_name =
                  star ? star->get_planet_name(p.planet_order()) : "Unknown",
              .distance = dist,
          },
  };
}

TacticalParams PlanetTacticalItem::get_tactical_params(const Race& race) const {
  return TacticalParams{
      .tech = race.tech,
      .weapon_range = race.gun_range(),
  };
}

bool PlanetTacticalItem::should_report_tactical(player_t player_num,
                                                governor_t) const {
  return planet_->info(player_num).numsectsowned != 0;
}

void generate_tactical_reports(
    GameObj& g, TacticalContext& ctx,
    const std::vector<std::unique_ptr<TacticalItem>>& items) {
  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  TacticalViewModel vm{};
  for (const auto& item : items) {
    if (!item->should_report_tactical(g.player(), g.governor())) {
      continue;
    }
    TacticalParams params = item->get_tactical_params(*race);
    vm.entries.push_back(
        item->build_report_entry(g, ctx, *race, items, params));
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
}

void collect_ship_area_items(GameObj& g,
                             std::vector<std::unique_ptr<TacticalItem>>& items,
                             const Ship& ship) {
  add_tactical_ship(items, &ship);
  if (ship.whatorbits() == ScopeLevel::LEVEL_STAR) {
    star_get_tactical_items(g, items, g.player(), ship.storbits());
  } else if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    plan_get_tactical_items(g, items, g.player(), ship.storbits(),
                            ship.pnumorbits());
  }
}

bool is_ship_number_arg(std::string_view arg) {
  return !arg.empty() &&
         (arg.front() == '#' ||
          std::isdigit(static_cast<unsigned char>(arg.front())));
}

void parse_tactical_filter_arg(std::string_view arg, TacticalContext& ctx) {
  if (!arg.empty() && std::isdigit(static_cast<unsigned char>(arg.front()))) {
    if (auto res = scn::scan<player_t::value_type>(arg, "{}")) {
      ctx.filter_player = player_t{res->value()};
    }
  } else {
    ctx.shiplist = std::string(arg);
  }
}

bool collect_explicit_ships_tactical(
    const command_t& argv, GameObj& g, TacticalContext& ctx,
    std::vector<std::unique_ptr<TacticalItem>>& items) {
  const shipnum_t n_ships = g.entity_manager.num_ships();
  std::size_t ship_arg_end = argv.size();
  if (argv.size() == 3 && !argv[2].empty() && argv[2].front() != '#') {
    parse_tactical_filter_arg(argv[2], ctx);
    ship_arg_end = 2;
  }

  for (std::size_t l = 1; l < ship_arg_end; ++l) {
    std::string_view arg_sv = argv[l];
    if (!arg_sv.empty() && arg_sv.front() == '#') {
      arg_sv.remove_prefix(1);
    }
    auto scan_res = scn::scan<underlying_type_t<shipnum_t>>(arg_sv, "{}");
    if (!scan_res) {
      g.out << std::format("tactical: invalid ship argument {}\n", argv[l]);
      return false;
    }
    shipnum_t shipno{scan_res->value()};
    const auto* ship = (shipno >= 1 && shipno <= n_ships)
                           ? g.entity_manager.peek_ship(shipno)
                           : nullptr;
    if (!ship) {
      g.out << std::format("tactical: no such ship #{} \n", shipno);
      return false;
    }
    collect_ship_area_items(g, items, *ship);
  }
  return true;
}

bool collect_scoped_tactical_items(
    GameObj& g, std::vector<std::unique_ptr<TacticalItem>>& items) {
  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't do tactical from universe level.\n";
      return false;
    case ScopeLevel::LEVEL_PLAN:
      plan_get_tactical_items(g, items, g.player(), g.snum(), g.pnum());
      return true;
    case ScopeLevel::LEVEL_STAR:
      star_get_tactical_items(g, items, g.player(), g.snum());
      return true;
    case ScopeLevel::LEVEL_SHIP: {
      if (g.shipno() == 0) {
        g.out << "Error: No ship is currently scoped. Use 'cs #<shipno>' to "
                 "scope to a ship.\n";
        return false;
      }
      const auto* scoped_ship = g.entity_manager.peek_ship(g.shipno());
      if (!scoped_ship) {
        g.out << std::format("Error: Unable to retrieve ship #{} data.\n",
                             g.shipno());
        return false;
      }
      collect_ship_area_items(g, items, *scoped_ship);
      return true;
    }
  }
  return false;
}

}  // namespace

namespace GB::commands {
bool tactical(const command_t& argv, GameObj& g) {
  TacticalContext ctx;
  std::vector<std::unique_ptr<TacticalItem>> items;

  if (argv.size() >= 2) {
    if (is_ship_number_arg(argv[1])) {
      if (!collect_explicit_ships_tactical(argv, g, ctx, items)) {
        return false;
      }
      generate_tactical_reports(g, ctx, items);
      return true;
    }
    ctx.shiplist = argv[1];
    if (argv.size() == 3) {
      parse_tactical_filter_arg(argv[2], ctx);
    }
  }

  if (!collect_scoped_tactical_items(g, items)) {
    return false;
  }

  generate_tactical_reports(g, ctx, items);
  return true;
}

const CommandDescriptor tactical_cmd{
    .name = "tactical",
    .roles = {},
    .scopes = {.star = true, .planet = true, .ship = true},
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "tactical [<#ship|shiptype>] [<race>]",
    .description =
        "Report on tactical combat capabilities and firing solutions",
    .handler = &tactical,
};

}  // namespace GB::commands
