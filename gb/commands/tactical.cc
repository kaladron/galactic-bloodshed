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
import gb.services;
import scnlib;
import std;
import tabulate;
#undef stdout

module commands;

namespace {

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

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

// Forward declarations
class TacticalItem;

// ============================================================================
// BASE CLASS: TacticalItem
// ============================================================================

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

  // Add header row to tactical summary table (polymorphic)
  virtual void add_tactical_header_row(tabulate::Table&, GameObj&, player_t,
                                       const TacticalParams&) const = 0;

  // Add target row to tactical targets table (polymorphic)
  virtual void add_tactical_target_row(tabulate::Table&, GameObj&,
                                       TacticalContext&, const Race&,
                                       double dist,
                                       const TacticalParams& firer) const = 0;

  // Get tactical parameters for this item
  virtual TacticalParams get_tactical_params(const Race&) const = 0;

  // Check if we should generate tactical report for this item
  virtual bool should_report_tactical(player_t player_num,
                                      governor_t governor) const = 0;

  // Generate tactical report for this item
  void report_tactical(GameObj& g, TacticalContext& ctx,
                       const std::vector<std::unique_ptr<TacticalItem>>& items,
                       const TacticalParams& params) const;
};

// ============================================================================
// DERIVED CLASSES
// ============================================================================

// Ship tactical item - holds non-owning pointer from peek_ship
class ShipTacticalItem : public TacticalItem {
  const Ship* ship_;

public:
  explicit ShipTacticalItem(const Ship* ship)
      : TacticalItem(ship->coordinates()), ship_(ship) {}

  std::optional<shipnum_t> ship_number() const override {
    return ship_->number();
  }

  void add_tactical_header_row(tabulate::Table& table, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const override;

  void add_tactical_target_row(tabulate::Table& table, GameObj& g,
                               TacticalContext& ctx, const Race& race,
                               double dist,
                               const TacticalParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report_tactical(player_t player_num,
                              governor_t governor) const override;
};

// Planet tactical item - holds non-owning pointer from peek_planet
class PlanetTacticalItem : public TacticalItem {
  const Planet* planet_;

public:
  PlanetTacticalItem(const Planet* planet, UniverseCoordinates coords)
      : TacticalItem(coords), planet_(planet) {}

  void add_tactical_header_row(tabulate::Table& table, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const override;

  void add_tactical_target_row(tabulate::Table& table, GameObj& g,
                               TacticalContext& ctx, const Race& race,
                               double dist,
                               const TacticalParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report_tactical(player_t player_num,
                              governor_t governor) const override;
};

// ============================================================================
// TABLE FORMATTING & HELPER FUNCTIONS
// ============================================================================

void configure_tactical_header_table(tabulate::Table& header_table) {
  header_table.format().hide_border().column_separator("  ");

  header_table.column(0).format().width(3).font_align(
      tabulate::FontAlign::right);
  header_table.column(1).format().width(1).font_align(
      tabulate::FontAlign::center);
  header_table.column(2).format().width(16);
  header_table.column(3).format().width(4).font_align(
      tabulate::FontAlign::right);
  header_table.column(4).format().width(7).font_align(
      tabulate::FontAlign::center);
  header_table.column(5).format().width(5).font_align(
      tabulate::FontAlign::right);
  header_table.column(6).format().width(4).font_align(
      tabulate::FontAlign::right);
  header_table.column(7).format().width(5).font_align(
      tabulate::FontAlign::right);
  header_table.column(8).format().width(7).font_align(
      tabulate::FontAlign::right);
  header_table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);
  header_table.column(10).format().width(3).font_align(
      tabulate::FontAlign::right);
  header_table.column(11).format().width(4).font_align(
      tabulate::FontAlign::center);
  header_table.column(12).format().width(30);

  header_table.add_row({"#", "", "name", "tech", "guns", "armor", "size",
                        "dest", "fuel", "dam", "spd", "evad", "orbits"});
  header_table[0].format().font_style({tabulate::FontStyle::bold});
}

void configure_tactical_targets_table(tabulate::Table& tactical_table) {
  tactical_table.format().hide_border().column_separator("  ");

  tactical_table.column(0).format().width(13);
  tactical_table.column(1).format().width(5).font_align(
      tabulate::FontAlign::center);
  tactical_table.column(2).format().width(3).font_align(
      tabulate::FontAlign::center);
  tactical_table.column(3).format().width(14);
  tactical_table.column(4).format().width(4).font_align(
      tabulate::FontAlign::right);
  tactical_table.column(5).format().width(4).font_align(
      tabulate::FontAlign::right);
  tactical_table.column(6).format().width(4).font_align(
      tabulate::FontAlign::right);
  tactical_table.column(7).format().width(3).font_align(
      tabulate::FontAlign::right);
  tactical_table.column(8).format().width(5).font_align(
      tabulate::FontAlign::center);
  tactical_table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);
  tactical_table.column(10).format().width(3).font_align(
      tabulate::FontAlign::right);
  tactical_table.column(11).format().width(10);

  tactical_table.add_row({"Tactical: #", "own", "typ", "name", "rng", "(50%)",
                          "size", "spd", "evade", "hit", "dam", "loc"});
  tactical_table[0].format().font_style({tabulate::FontStyle::bold});
}

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

    for (planetnum_t i = 0; i < star->numplanets(); i++)
      plan_get_tactical_items(g, items, player_num, snum, i);
  }
}

void TacticalItem::report_tactical(
    GameObj& g, TacticalContext& ctx,
    const std::vector<std::unique_ptr<TacticalItem>>& items,
    const TacticalParams& params) const {
  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) return;

  tabulate::Table header_table;
  configure_tactical_header_table(header_table);
  add_tactical_header_row(header_table, g, g.player(), params);
  g.out << "\n" << header_table << "\n";

  tabulate::Table tactical_table;
  configure_tactical_targets_table(tactical_table);

  for (const auto& target : items) {
    if (target.get() == this) continue;

    const double dist = coords_.distance_to(target->coordinates());
    if (dist >= params.weapon_range) continue;

    target->add_tactical_target_row(tactical_table, g, ctx, *race, dist,
                                    params);
  }

  if (tactical_table.size() > 1) {
    g.out << "\n" << tactical_table << "\n";
  }
}

// ============================================================================
// SHIP TACTICAL IMPLEMENTATIONS
// ============================================================================

void ShipTacticalItem::add_tactical_header_row(
    tabulate::Table& table, GameObj& g, player_t,
    const TacticalParams& params) const {
  const auto& s = *ship_;
  std::string name_str = s.active() ? s.name() : "INACTIVE";
  std::string orbits_str = dispshiploc(g.entity_manager, s);

  std::string location_suffix;
  if (s.is_landed()) {
    location_suffix = std::format(" ({})", s.land_coords());
  }

  std::string inactive_suffix;
  if (!s.active()) {
    inactive_suffix = std::format(" INACTIVE({})", s.rad());
  }

  table.add_row(
      {std::format("{}", s.number()), std::format("{}", s.type_letter()),
       name_str, std::format("{:.0f}", s.tech()), s.battery_summary(),
       std::format("{}", s.armor()), std::format("{}", s.size()),
       std::format("{}", s.destruct()), std::format("{:.1f}", s.fuel()),
       std::format("{}%", s.damage()), std::format("{}", params.fspeed),
       params.fev ? "yes" : "",
       std::format("{}{}{}", orbits_str, location_suffix, inactive_suffix)});
}

bool is_valid_ship_target(const Ship& s, const GameObj& g,
                          const TacticalContext& ctx, const Race& race) {
  if (ctx.filter_player.has_value() && s.owner() != *ctx.filter_player) {
    return false;
  }
  if (!ctx.shiplist.empty() && !listed(s.type(), ctx.shiplist)) {
    return false;
  }
  if (s.owner() == g.player() && authorized(g.governor(), s)) {
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

void ShipTacticalItem::add_tactical_target_row(
    tabulate::Table& table, GameObj& g, TacticalContext& ctx, const Race& race,
    double dist, const TacticalParams& firer) const {
  const auto& s = *ship_;
  if (!is_valid_ship_target(s, g, ctx, race)) {
    return;
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

  const auto* war_status = race.is_at_war_with(s.owner())   ? "-"
                           : race.is_allied_with(s.owner()) ? "+"
                                                            : " ";

  std::string loc_str =
      s.is_landed() ? std::format("{}", s.land_coords()) : std::string{};
  std::string status_suffix = s.active() ? "" : " INACTIVE";

  table.add_row({std::format("{}", s.number()),
                 std::format("{}{},{}", war_status, s.owner(), s.governor()),
                 std::format("{}", s.type_letter()),
                 std::format("{:.14}", s.name()), std::format("{:.0f}", dist),
                 std::format("{}", factor), std::format("{}", body),
                 std::format("{}", tspeed), tev ? "yes" : "",
                 std::format("{}%", prob), std::format("{}%", s.damage()),
                 std::format("{}{}", loc_str, status_suffix)});
}

TacticalParams ShipTacticalItem::get_tactical_params(const Race&) const {
  const auto& s = *ship_;
  TacticalParams params{
      .tech = s.tech(),
      .weapon_range = s.gun_range(),
      .damage = s.damage(),
      .caliber = current_caliber(s),
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
  return s.alive() && s.owner() == player_num && authorized(governor, s) &&
         s.has_sight();
}

// ============================================================================
// PLANET TACTICAL IMPLEMENTATIONS
// ============================================================================

void PlanetTacticalItem::add_tactical_header_row(
    tabulate::Table& table, GameObj& g, player_t player_num,
    const TacticalParams& params) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id());

  std::string name_str = std::format(
      "(planet){}", star ? star->get_planet_name(p.planet_order()) : "Unknown");

  table.add_row({"", "", name_str, std::format("{:.0f}", params.tech),
                 std::format("{}M", p.info(player_num).guns), "", "",
                 std::format("{}", p.info(player_num).destruct),
                 std::format("{}", p.info(player_num).fuel), "", "", "", ""});
}

void PlanetTacticalItem::add_tactical_target_row(tabulate::Table& table,
                                                 GameObj& g, TacticalContext&,
                                                 const Race&, double dist,
                                                 const TacticalParams&) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id());
  std::string name_str =
      star ? star->get_planet_name(p.planet_order()) : "Unknown";

  table.add_row({"", "(planet)", "", name_str, std::format("{:.0f}", dist), "",
                 "", "", "", "", "", ""});
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

// ============================================================================
// TACTICAL REPORT DRIVER & COMMAND HELPERS
// ============================================================================

void generate_tactical_reports(
    GameObj& g, TacticalContext& ctx,
    const std::vector<std::unique_ptr<TacticalItem>>& items) {
  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  for (const auto& item : items) {
    if (!item->should_report_tactical(g.player(), g.governor())) {
      continue;
    }
    TacticalParams params = item->get_tactical_params(*race);
    item->report_tactical(g, ctx, items, params);
  }
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

// ============================================================================
// COMMAND ENTRY POINT
// ============================================================================

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
