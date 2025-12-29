// SPDX-License-Identifier: Apache-2.0

/// \file tactical.cc
/// \brief Tactical combat reporting command
///
/// Tactical reports show firing solutions, hit probabilities, and target
/// information for combat scenarios. Unlike other reports, tactical requires
/// collecting all ships and planets in range to calculate distance-based
/// firing solutions.

module;

import gblib;
import std.compat;
import tabulate;

module commands;

namespace {

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

struct TacticalParams {
  double tech = 0.0;
  bool fev = false;
  int fspeed = 0;
};

// Parameters for the firing ship when calculating hit odds against targets
struct FiringShipParams {
  double tech = 0.0;
  int damage = 0;
  bool evade = false;
  int speed = 0;
  guntype_t caliber = GTYPE_NONE;
  bool laser_focused = false;  // Laser is on and focused
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
  const double x_;
  const double y_;

public:
  TacticalItem(double x, double y) : x_(x), y_(y) {}
  virtual ~TacticalItem() = default;

  // Non-copyable and non-movable (polymorphic base class with unique ownership)
  TacticalItem(const TacticalItem&) = delete;
  TacticalItem& operator=(const TacticalItem&) = delete;
  TacticalItem(TacticalItem&&) = delete;
  TacticalItem& operator=(TacticalItem&&) = delete;

  double x() const {
    return x_;
  }
  double y() const {
    return y_;
  }

  // Add header row to tactical summary table (polymorphic)
  virtual void add_tactical_header_row(tabulate::Table&, GameObj&, player_t,
                                       const TacticalParams&) const = 0;

  // Add target row to tactical targets table (polymorphic)
  virtual void
  add_tactical_target_row(tabulate::Table&, GameObj&, TacticalContext&,
                          const std::vector<std::unique_ptr<TacticalItem>>&,
                          const Race&, double dist,
                          const FiringShipParams& firer) const = 0;

  // Get tactical parameters for this item
  virtual TacticalParams get_tactical_params(const Race&) const {
    return TacticalParams{};
  }

  // Check if we should generate tactical report for this item
  virtual bool should_report_tactical(player_t player_num,
                                      governor_t governor) const = 0;

  // Generate tactical report for this item
  virtual void
  report_tactical(GameObj&, TacticalContext&,
                  const std::vector<std::unique_ptr<TacticalItem>>&,
                  const TacticalParams&) const = 0;
};

// ============================================================================
// DERIVED CLASSES
// ============================================================================

// Ship tactical item - holds non-owning pointer from peek_ship
class ShipTacticalItem : public TacticalItem {
  const Ship* ship_;

public:
  explicit ShipTacticalItem(const Ship* ship)
      : TacticalItem(ship->xpos(), ship->ypos()), ship_(ship) {}

  const Ship& ship() const {
    return *ship_;
  }

  void add_tactical_header_row(tabulate::Table& table, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const override;

  void add_tactical_target_row(
      tabulate::Table& table, GameObj& g, TacticalContext& ctx,
      const std::vector<std::unique_ptr<TacticalItem>>& items, const Race& race,
      double dist, const FiringShipParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report_tactical(player_t player_num,
                              governor_t governor) const override;

  void report_tactical(GameObj& g, TacticalContext& ctx,
                       const std::vector<std::unique_ptr<TacticalItem>>& items,
                       const TacticalParams& params) const override;
};

// Planet tactical item - holds non-owning pointer from peek_planet
class PlanetTacticalItem : public TacticalItem {
  const Planet* planet_;

public:
  PlanetTacticalItem(const Planet* planet, double x, double y)
      : TacticalItem(x, y), planet_(planet) {}

  starnum_t star() const {
    return planet_->star_id();
  }
  planetnum_t pnum() const {
    return planet_->planet_order();
  }
  const Planet& planet() const {
    return *planet_;
  }

  void add_tactical_header_row(tabulate::Table& table, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const override;

  void add_tactical_target_row(
      tabulate::Table& table, GameObj& g, TacticalContext& ctx,
      const std::vector<std::unique_ptr<TacticalItem>>& items, const Race& race,
      double dist, const FiringShipParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report_tactical(player_t player_num,
                              governor_t governor) const override;

  void report_tactical(GameObj& g, TacticalContext& ctx,
                       const std::vector<std::unique_ptr<TacticalItem>>& items,
                       const TacticalParams& params) const override;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/* get a ship and add it to the tactical item list */
void add_tactical_ship(std::vector<std::unique_ptr<TacticalItem>>& items,
                       const Ship* ship) {
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
  double x = star->xpos() + planet->xpos();
  double y = star->ypos() + planet->ypos();
  items.push_back(std::make_unique<PlanetTacticalItem>(planet, x, y));

  if (planet->info(player_num - 1).explored) {
    const ShipList ships(g.entity_manager, planet->ships());
    for (const Ship* ship : ships) {
      add_tactical_ship(items, ship);
    }
  }
}

void star_get_tactical_items(GameObj& g,
                             std::vector<std::unique_ptr<TacticalItem>>& items,
                             player_t player_num, starnum_t snum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (isset(star->explored(), player_num)) {
    const ShipList ships(g.entity_manager, star->ships());
    for (const Ship* ship : ships) {
      add_tactical_ship(items, ship);
    }

    for (planetnum_t i = 0; i < star->numplanets(); i++)
      plan_get_tactical_items(g, items, player_num, snum, i);
  }
}

// ============================================================================
// SHIP TACTICAL IMPLEMENTATIONS
// ============================================================================

void ShipTacticalItem::add_tactical_header_row(
    tabulate::Table& table, GameObj&, player_t,
    const TacticalParams& params) const {
  const auto& s = *ship_;
  Place where{s.whatorbits(), s.storbits(), s.pnumorbits()};

  std::string name_str = s.active() ? s.name() : "INACTIVE";
  std::string orbits_str = where.to_string();

  // Build landed location suffix
  std::string location_suffix;
  if (landed(s)) {
    location_suffix = std::format(" ({},{})", s.land_x(), s.land_y());
  }

  // Build inactive suffix
  std::string inactive_suffix;
  if (!s.active()) {
    inactive_suffix = std::format(" INACTIVE({})", s.rad());
  }

  table.add_row(
      {std::format("{}", s.number()), std::format("{}", Shipltrs[s.type()]),
       name_str, std::format("{:.0f}", s.tech()),
       std::format("{}{}/{}{}", s.primary(), caliber_char(s.primtype()),
                   s.secondary(), caliber_char(s.sectype())),
       std::format("{}", s.armor()), std::format("{}", s.size()),
       std::format("{}", s.destruct()), std::format("{:.1f}", s.fuel()),
       std::format("{}%", s.damage()), std::format("{}", params.fspeed),
       params.fev ? "yes" : "",
       std::format("{}{}{}", orbits_str, location_suffix, inactive_suffix)});
}

void ShipTacticalItem::add_tactical_target_row(
    tabulate::Table& table, GameObj& g, TacticalContext& ctx,
    const std::vector<std::unique_ptr<TacticalItem>>&, const Race& race,
    double dist, const FiringShipParams& firer) const {
  const auto& s = *ship_;

  // Filter ships based on player filter or ship type list
  // If filter_player is set, only show ships owned by that player
  // If shiplist is non-empty, only show ships whose type is in the list
  if (ctx.filter_player.has_value()) {
    // Player filter mode: show only ships owned by specified player
    if (s.owner() != *ctx.filter_player) {
      return;
    }
  } else if (!ctx.shiplist.empty()) {
    // Ship type filter mode: show only ships whose type is in the list
    if (!listed(s.type(), ctx.shiplist)) {
      return;
    }
  }

  // Don't show ships we own and are authorized for
  if (s.owner() == g.player() && authorized(g.governor(), s)) {
    return;
  }

  // Don't show dead ships
  if (!s.alive()) {
    return;
  }

  // Don't show canisters or greenhouse gases
  if (s.type() == ShipType::OTYPE_CANIST || s.type() == ShipType::OTYPE_GREEN) {
    return;
  }

  // Calculate target ship's evasion and speed (only if moving and active)
  bool tev = false;
  int tspeed = 0;
  if ((s.whatdest() != ScopeLevel::LEVEL_UNIV || s.navigate().on) &&
      !s.docked() && s.active()) {
    tspeed = s.speed();
    tev = s.protect().evade;
  }

  // Calculate combat parameters using firer's data and target's data
  int body = size(s);
  auto defense = getdefense(g.entity_manager, s);
  auto [prob, factor] =
      hit_odds(dist, firer.tech, firer.damage, firer.evade, tev, firer.speed,
               tspeed, body, firer.caliber, defense);

  // Apply laser focus bonus if firer has it
  if (firer.laser_focused) {
    prob = prob * prob / 100;
  }

  // Determine diplomatic status indicator
  const auto* war_status = isset(race.atwar, s.owner())    ? "-"
                           : isset(race.allied, s.owner()) ? "+"
                                                           : " ";

  // Filter out allied ships if enemies-only mode is enabled
  if (ctx.enemies_only && isset(race.allied, s.owner())) {
    return;
  }

  // Build location string
  std::string loc_str;
  if (landed(s)) {
    loc_str = std::format("({},{})", s.land_x(), s.land_y());
  }

  // Build status suffix
  std::string status_suffix = s.active() ? "" : " INACTIVE";

  // Add row to table
  table.add_row({std::format("{}", s.number()),
                 std::format("{}{},{}", war_status, s.owner(), s.governor()),
                 std::format("{}", Shipltrs[s.type()]),
                 std::format("{:.14}", s.name()), std::format("{:.0f}", dist),
                 std::format("{}", factor), std::format("{}", body),
                 std::format("{}", tspeed), tev ? "yes" : "",
                 std::format("{}%", prob), std::format("{}%", s.damage()),
                 std::format("{}{}", loc_str, status_suffix)});
}

TacticalParams ShipTacticalItem::get_tactical_params(const Race&) const {
  TacticalParams params{};
  const auto& s = *ship_;
  params.tech = s.tech();

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

  // Don't report on ships that are dead
  if (!s.alive()) return false;

  // Don't report on ships not owned by this player
  if (s.owner() != player_num) return false;

  // Don't report on ships this governor is not authorized for
  if (!authorized(governor, s)) return false;

  // Don't report on ships without sight capability
  if (!shipsight(s)) return false;

  return true;
}

void ShipTacticalItem::report_tactical(
    GameObj& g, TacticalContext& ctx,
    const std::vector<std::unique_ptr<TacticalItem>>& items,
    const TacticalParams& params) const {
  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) return;

  const auto& s = *ship_;

  // Create header summary table
  tabulate::Table header_table;
  header_table.format().hide_border().column_separator("  ");

  // Configure column widths and alignments for header table
  header_table.column(0).format().width(3).font_align(
      tabulate::FontAlign::right);  // #
  header_table.column(1).format().width(1).font_align(
      tabulate::FontAlign::center);           // type
  header_table.column(2).format().width(16);  // name
  header_table.column(3).format().width(4).font_align(
      tabulate::FontAlign::right);  // tech
  header_table.column(4).format().width(7).font_align(
      tabulate::FontAlign::center);  // guns
  header_table.column(5).format().width(5).font_align(
      tabulate::FontAlign::right);  // armor
  header_table.column(6).format().width(4).font_align(
      tabulate::FontAlign::right);  // size
  header_table.column(7).format().width(5).font_align(
      tabulate::FontAlign::right);  // dest
  header_table.column(8).format().width(7).font_align(
      tabulate::FontAlign::right);  // fuel
  header_table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);  // dam
  header_table.column(10).format().width(3).font_align(
      tabulate::FontAlign::right);  // spd
  header_table.column(11).format().width(4).font_align(
      tabulate::FontAlign::center);            // evad
  header_table.column(12).format().width(30);  // orbits

  // Add header row
  header_table.add_row({"#", "", "name", "tech", "guns", "armor", "size",
                        "dest", "fuel", "dam", "spd", "evad", "orbits"});
  header_table[0].format().font_style({tabulate::FontStyle::bold});

  // Add data row polymorphically
  add_tactical_header_row(header_table, g, g.player(), params);

  g.out << "\n" << header_table << "\n";

  // Create tactical targets table
  tabulate::Table tactical_table;
  tactical_table.format().hide_border().column_separator("  ");

  // Configure column widths and alignments for tactical table
  tactical_table.column(0).format().width(13);  // #
  tactical_table.column(1).format().width(5).font_align(
      tabulate::FontAlign::center);  // own
  tactical_table.column(2).format().width(3).font_align(
      tabulate::FontAlign::center);             // typ
  tactical_table.column(3).format().width(14);  // name
  tactical_table.column(4).format().width(4).font_align(
      tabulate::FontAlign::right);  // rng
  tactical_table.column(5).format().width(4).font_align(
      tabulate::FontAlign::right);  // (50%)
  tactical_table.column(6).format().width(4).font_align(
      tabulate::FontAlign::right);  // size
  tactical_table.column(7).format().width(3).font_align(
      tabulate::FontAlign::right);  // spd
  tactical_table.column(8).format().width(5).font_align(
      tabulate::FontAlign::center);  // evade
  tactical_table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);  // hit
  tactical_table.column(10).format().width(3).font_align(
      tabulate::FontAlign::right);               // dam
  tactical_table.column(11).format().width(10);  // loc

  // Add tactical header row
  tactical_table.add_row({"Tactical: #", "own", "typ", "name", "rng", "(50%)",
                          "size", "spd", "evade", "hit", "dam", "loc"});
  tactical_table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& target : items) {
    // Skip ourselves
    if (target->x() == x_ && target->y() == y_) continue;

    double range = gun_range(s);
    double dist = std::hypot(x_ - target->x(), y_ - target->y());
    if (dist >= range) continue;

    // Build firing ship parameters
    FiringShipParams firer{
        .tech = params.tech,
        .damage = s.damage(),
        .evade = params.fev,
        .speed = params.fspeed,
        .caliber = current_caliber(s),
        .laser_focused = (laser_on(s) && s.focus()),
    };

    // Polymorphic call - adds row to table for ships, skips for planets not in
    // range
    target->add_tactical_target_row(tactical_table, g, ctx, items, *race, dist,
                                    firer);
  }

  // Only output tactical table if we have targets
  if (tactical_table.size() > 1) {  // More than just the header
    g.out << "\n" << tactical_table << "\n";
  }
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
                 std::format("{}M", p.info(player_num - 1).guns), "", "",
                 std::format("{}", p.info(player_num - 1).destruct),
                 std::format("{}", p.info(player_num - 1).fuel), "", "", "",
                 ""});
}

void PlanetTacticalItem::add_tactical_target_row(
    tabulate::Table& table, GameObj& g, TacticalContext&,
    const std::vector<std::unique_ptr<TacticalItem>>&, const Race&, double dist,
    const FiringShipParams&) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id());
  std::string name_str =
      star ? star->get_planet_name(p.planet_order()) : "Unknown";

  table.add_row({"", "(planet)", "", name_str, std::format("{:.0f}", dist), "",
                 "", "", "", "", "", ""});
}

TacticalParams PlanetTacticalItem::get_tactical_params(const Race& race) const {
  TacticalParams params{};
  params.tech = race.tech;
  // Planets don't have speed or evasion
  return params;
}

bool PlanetTacticalItem::should_report_tactical(player_t player_num,
                                                governor_t) const {
  // Report on planets where we own sectors
  return planet_->info(player_num - 1).numsectsowned != 0;
}

void PlanetTacticalItem::report_tactical(
    GameObj& g, TacticalContext& ctx,
    const std::vector<std::unique_ptr<TacticalItem>>& items,
    const TacticalParams& params) const {
  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) return;

  // Create header summary table
  tabulate::Table header_table;
  header_table.format().hide_border().column_separator("  ");

  // Configure column widths and alignments for header table
  header_table.column(0).format().width(3).font_align(
      tabulate::FontAlign::right);  // #
  header_table.column(1).format().width(1).font_align(
      tabulate::FontAlign::center);           // type
  header_table.column(2).format().width(16);  // name
  header_table.column(3).format().width(4).font_align(
      tabulate::FontAlign::right);  // tech
  header_table.column(4).format().width(7).font_align(
      tabulate::FontAlign::center);  // guns
  header_table.column(5).format().width(5).font_align(
      tabulate::FontAlign::right);  // armor
  header_table.column(6).format().width(4).font_align(
      tabulate::FontAlign::right);  // size
  header_table.column(7).format().width(5).font_align(
      tabulate::FontAlign::right);  // dest
  header_table.column(8).format().width(7).font_align(
      tabulate::FontAlign::right);  // fuel
  header_table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);  // dam
  header_table.column(10).format().width(3).font_align(
      tabulate::FontAlign::right);  // spd
  header_table.column(11).format().width(4).font_align(
      tabulate::FontAlign::center);            // evad
  header_table.column(12).format().width(30);  // orbits

  // Add header row
  header_table.add_row({"#", "", "name", "tech", "guns", "armor", "size",
                        "dest", "fuel", "dam", "spd", "evad", "orbits"});
  header_table[0].format().font_style({tabulate::FontStyle::bold});

  // Add data row polymorphically
  add_tactical_header_row(header_table, g, g.player(), params);

  g.out << "\n" << header_table << "\n";

  // Create tactical targets table
  tabulate::Table tactical_table;
  tactical_table.format().hide_border().column_separator("  ");

  // Configure column widths and alignments for tactical table
  tactical_table.column(0).format().width(13);  // #
  tactical_table.column(1).format().width(5).font_align(
      tabulate::FontAlign::center);  // own
  tactical_table.column(2).format().width(3).font_align(
      tabulate::FontAlign::center);             // typ
  tactical_table.column(3).format().width(14);  // name
  tactical_table.column(4).format().width(4).font_align(
      tabulate::FontAlign::right);  // rng
  tactical_table.column(5).format().width(4).font_align(
      tabulate::FontAlign::right);  // (50%)
  tactical_table.column(6).format().width(4).font_align(
      tabulate::FontAlign::right);  // size
  tactical_table.column(7).format().width(3).font_align(
      tabulate::FontAlign::right);  // spd
  tactical_table.column(8).format().width(5).font_align(
      tabulate::FontAlign::center);  // evade
  tactical_table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);  // hit
  tactical_table.column(10).format().width(3).font_align(
      tabulate::FontAlign::right);               // dam
  tactical_table.column(11).format().width(10);  // loc

  // Add tactical header row
  tactical_table.add_row({"Tactical: #", "own", "typ", "name", "rng", "(50%)",
                          "size", "spd", "evade", "hit", "dam", "loc"});
  tactical_table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& target : items) {
    // Skip ourselves
    if (target->x() == x_ && target->y() == y_) continue;

    double range = gun_range(*race);
    double dist = std::hypot(x_ - target->x(), y_ - target->y());
    if (dist >= range) continue;

    // Build firing ship parameters from planet perspective
    FiringShipParams firer{
        .tech = params.tech,
        .damage = 0,  // Planets don't have damage
        .evade = params.fev,
        .speed = params.fspeed,
        .caliber = GTYPE_MEDIUM,
        .laser_focused = false,  // Planets don't have laser focus
    };

    // Polymorphic call - adds row to table for ships, skips for planets not in
    // range
    target->add_tactical_target_row(tactical_table, g, ctx, items, *race, dist,
                                    firer);
  }

  // Only output tactical table if we have targets
  if (tactical_table.size() > 1) {  // More than just the header
    g.out << "\n" << tactical_table << "\n";
  }
}

// ============================================================================
// TACTICAL REPORT DRIVER
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
    // Check if this item should generate a tactical report
    if (!item->should_report_tactical(g.player(), g.governor())) {
      continue;
    }

    // Get tactical parameters for this item
    TacticalParams params = item->get_tactical_params(*race);

    // Generate the tactical report
    item->report_tactical(g, ctx, items, params);
  }
}

}  // namespace

// ============================================================================
// COMMAND ENTRY POINT
// ============================================================================

namespace GB::commands {
void tactical(const command_t& argv, GameObj& g) {
  TacticalContext ctx;
  ctx.enemies_only = false;
  std::vector<std::unique_ptr<TacticalItem>> items;

  // Parse arguments
  if (argv.size() == 3) {
    if (isdigit(argv[2][0])) {
      // Filter by player number
      ctx.filter_player = std::stoi(argv[2]);
    } else {
      // Filter by ship type list (no player filter)
      ctx.shiplist = argv[2];
    }
  } else {
    // No filtering
    ctx.filter_player = std::nullopt;
  }

  // Handle specific ship number(s)
  if (argv.size() >= 2) {
    if (*argv[1].c_str() == '#' || isdigit(*argv[1].c_str())) {
      shipnum_t n_ships = g.entity_manager.num_ships();
      int l = 1;
      while (l < MAXARGS && *argv[l].c_str() != '\0') {
        shipnum_t shipno;
        sscanf(argv[l].c_str() + (*argv[l].c_str() == '#'), "%lu", &shipno);
        if (shipno > n_ships || shipno < 1) {
          g.out << std::format("tactical: no such ship #{} \n", shipno);
          return;
        }

        const auto* ship = g.entity_manager.peek_ship(shipno);
        if (!ship) {
          g.out << std::format("tactical: no such ship #{} \n", shipno);
          return;
        }

        // For tactical, we need to collect nearby ships/planets too
        // First add the target ship
        add_tactical_ship(items, ship);

        // Then collect ships/planets in the same area based on ship's location
        if (ship->whatorbits() == ScopeLevel::LEVEL_STAR) {
          star_get_tactical_items(g, items, g.player(), ship->storbits());
        } else if (ship->whatorbits() == ScopeLevel::LEVEL_PLAN) {
          plan_get_tactical_items(g, items, g.player(), ship->storbits(),
                                  ship->pnumorbits());
        }

        l++;
      }
      generate_tactical_reports(g, ctx, items);
      return;
    }

    // argv[1] might be ship type filter - store for target filtering
    ctx.shiplist = argv[1];
  }

  // Collect items based on current scope
  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't do tactical from universe level.\n";
      return;

    case ScopeLevel::LEVEL_PLAN:
      plan_get_tactical_items(g, items, g.player(), g.snum(), g.pnum());
      break;

    case ScopeLevel::LEVEL_STAR:
      star_get_tactical_items(g, items, g.player(), g.snum());
      break;

    case ScopeLevel::LEVEL_SHIP:
      if (g.shipno() == 0) {
        g.out << "Error: No ship is currently scoped. Use 'cs #<shipno>' to "
                 "scope to a ship.\n";
        return;
      }

      {
        const auto* scoped_ship = g.entity_manager.peek_ship(g.shipno());
        if (!scoped_ship) {
          g.out << std::format("Error: Unable to retrieve ship #{} data.\n",
                               g.shipno());
          return;
        }

        // Add the scoped ship
        add_tactical_ship(items, scoped_ship);

        // Also collect ships in the same area for targets (per documentation:
        // "Enemy ships will only appear on tactical display if they are in the
        // same scope as the calling ship")
        if (scoped_ship->whatorbits() == ScopeLevel::LEVEL_STAR) {
          star_get_tactical_items(g, items, g.player(),
                                  scoped_ship->storbits());
        } else if (scoped_ship->whatorbits() == ScopeLevel::LEVEL_PLAN) {
          plan_get_tactical_items(g, items, g.player(), scoped_ship->storbits(),
                                  scoped_ship->pnumorbits());
        }
      }
      break;
  }

  generate_tactical_reports(g, ctx, items);
}
}  // namespace GB::commands
