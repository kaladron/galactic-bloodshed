// SPDX-License-Identifier: Apache-2.0

/// \file rst.cc
/// \brief Ship and planet reporting commands (report, stock, tactical, stats,
///        weapons, factories)
///
/// Implements various reporting modes for ships and planets including status,
/// stock levels, tactical combat information, and factory configurations.

module;

import gblib;
import std.compat;
import tabulate;

module commands;

namespace {

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

using ReportSet = std::unordered_set<char>;

// Forward declarations
class ReportItem;
struct RstContext;

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

struct ReportFlags {
  bool status = false;
  bool ship = false;
  bool stock = false;
  bool report = false;
  bool weapons = false;
  bool factories = false;
  bool tactical = false;
};

struct RstContext {
  std::vector<std::unique_ptr<ReportItem>> rd;
  std::string shiplist;
  ReportFlags flags;
  bool first;
  bool enemies_only;
  std::optional<player_t>
      filter_player;  // Filter by player number (nullopt = no filter)
};

// ============================================================================
// BASE CLASS: ReportItem
// ============================================================================

class ReportItem {
 protected:
  const double x_;
  const double y_;

 public:
  ReportItem(double x, double y) : x_(x), y_(y) {}
  virtual ~ReportItem() = default;

  // Non-copyable and non-movable (polymorphic base class with unique ownership)
  ReportItem(const ReportItem&) = delete;
  ReportItem& operator=(const ReportItem&) = delete;
  ReportItem(ReportItem&&) = delete;
  ReportItem& operator=(ReportItem&&) = delete;

  double x() const { return x_; }
  double y() const { return y_; }

  // Report generation methods
  virtual void report_stock(GameObj&, RstContext&) const {}
  virtual void report_status(GameObj&, RstContext&) const {}
  virtual void report_weapons(GameObj&, RstContext&) const {}
  virtual void report_factories(GameObj&, RstContext&) const {}
  virtual void report_general(GameObj&, RstContext&) const {}
  virtual void report_tactical(GameObj&, RstContext&,
                               const TacticalParams&) const {}

  // Add header row to tactical summary table (polymorphic)
  virtual void add_tactical_header_row(tabulate::Table&, GameObj&, player_t,
                                       const TacticalParams&) const = 0;

  // Add target row to tactical targets table (polymorphic)
  virtual void add_tactical_target_row(tabulate::Table&, GameObj&, RstContext&,
                                       const Race&, double dist,
                                       const FiringShipParams& firer) const = 0;

  // Get tactical parameters for this item
  virtual TacticalParams get_tactical_params(const Race&) const {
    return TacticalParams{};
  }

  // Get star number if ship is in a star system (ships only)
  virtual std::optional<starnum_t> get_star_orbit() const {
    return std::nullopt;
  }

  // Get next ship in linked list (ships only)
  virtual shipnum_t next_ship() const { return 0; }

  // Get ships docked/landed on this item (ships only)
  virtual shipnum_t child_ships() const { return 0; }

  // Check if we should report this item
  virtual bool should_report(player_t player_num, governor_t governor,
                             const ReportSet& rep_on) const = 0;
};

// ============================================================================
// DERIVED CLASSES
// ============================================================================

// Ship report item - holds non-owning pointer from peek_ship
class ShipReportItem : public ReportItem {
  const Ship* ship_;

 public:
  ShipReportItem(const Ship* ship)
      : ReportItem(ship->xpos, ship->ypos), ship_(ship) {}

  const Ship& ship() const { return *ship_; }

  void report_stock(GameObj& g, RstContext& ctx) const override;
  void report_status(GameObj& g, RstContext& ctx) const override;
  void report_weapons(GameObj& g, RstContext& ctx) const override;
  void report_factories(GameObj& g, RstContext& ctx) const override;
  void report_general(GameObj& g, RstContext& ctx) const override;
  void report_tactical(GameObj& g, RstContext& ctx,
                       const TacticalParams& params) const override;

  void add_tactical_header_row(tabulate::Table& table, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const override;

  void add_tactical_target_row(tabulate::Table& table, GameObj& g,
                               RstContext& ctx, const Race& race, double dist,
                               const FiringShipParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  std::optional<starnum_t> get_star_orbit() const override;

  shipnum_t next_ship() const override { return ship_->nextship; }

  shipnum_t child_ships() const override { return ship_->ships; }

  bool should_report(player_t player_num, governor_t governor,
                     const ReportSet& rep_on) const override;
};

// Planet report item - holds non-owning pointer from peek_planet
class PlanetReportItem : public ReportItem {
  const Planet* planet_;

 public:
  PlanetReportItem(const Planet* planet, double x, double y)
      : ReportItem(x, y), planet_(planet) {}

  starnum_t star() const { return planet_->star_id; }
  planetnum_t pnum() const { return planet_->planet_order; }
  const Planet& planet() const { return *planet_; }

  void report_tactical(GameObj& g, RstContext& ctx,
                       const TacticalParams& params) const override;

  void add_tactical_header_row(tabulate::Table& table, GameObj& g,
                               player_t player_num,
                               const TacticalParams& params) const override;

  void add_tactical_target_row(tabulate::Table& table, GameObj& g,
                               RstContext& ctx, const Race& race, double dist,
                               const FiringShipParams& firer) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report(player_t player_num, governor_t governor,
                     const ReportSet& rep_on) const override;
};

// Map of command names to their report flag configurations
// TODO(jeffbailey): Replace with std::unordered_map when available in C++20
// modules
constexpr auto kCommandReportModes = std::array{
    std::pair{"report", ReportFlags{.report = true}},
    std::pair{"stock", ReportFlags{.stock = true}},
    std::pair{"tactical", ReportFlags{.tactical = true}},
    std::pair{"ship", ReportFlags{.status = true,
                                  .ship = true,
                                  .stock = true,
                                  .report = true,
                                  .weapons = true,
                                  .factories = true}},
    std::pair{"stats", ReportFlags{.status = true}},
    std::pair{"weapons", ReportFlags{.weapons = true}},
    std::pair{"factories", ReportFlags{.factories = true}},
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/// Get display character for gun caliber type
/// \param caliber Gun caliber type (GTYPE_NONE=0, GTYPE_LIGHT=1,
/// GTYPE_MEDIUM=2, GTYPE_HEAVY=3)
/// \return Character representing caliber ('L', 'M', 'H', or ' ' for none)
constexpr char caliber_char(guntype_t caliber) {
  switch (caliber) {
    case GTYPE_LIGHT:
      return 'L';
    case GTYPE_MEDIUM:
      return 'M';
    case GTYPE_HEAVY:
      return 'H';
    case GTYPE_NONE:
    default:
      return ' ';
  }
}

bool listed(ShipType type, const std::string& string) {
  return std::ranges::any_of(string,
                             [type](char c) { return Shipltrs[type] == c; });
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
bool get_report_ship(GameObj& g, RstContext& ctx, shipnum_t shipno) {
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (ship) {
    ctx.rd.push_back(std::make_unique<ShipReportItem>(ship));
    return true;
  }
  g.out << std::format("get_report_ship: error on ship get ({}).\n", shipno);
  return false;
}

void plan_get_report_ships(GameObj& g, RstContext& ctx, player_t player_num,
                           starnum_t snum, planetnum_t pnum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  const auto* planet = g.entity_manager.peek_planet(snum, pnum);
  if (!planet) return;

  // Add planet to report list
  double x = star->xpos + planet->xpos;
  double y = star->ypos + planet->ypos;
  ctx.rd.push_back(std::make_unique<PlanetReportItem>(planet, x, y));

  if (planet->info[player_num - 1].explored) {
    shipnum_t shn = planet->ships;
    while (shn && get_report_ship(g, ctx, shn)) {
      shn = ctx.rd.back()->next_ship();
    }
  }
}

void star_get_report_ships(GameObj& g, RstContext& ctx, player_t player_num,
                           starnum_t snum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (isset(star->explored, player_num)) {
    shipnum_t shn = star->ships;
    while (shn && get_report_ship(g, ctx, shn)) {
      shn = ctx.rd.back()->next_ship();
    }
    for (planetnum_t i = 0; i < star->numplanets; i++)
      plan_get_report_ships(g, ctx, player_num, snum, i);
  }
}

// ============================================================================
// SHIP REPORT METHOD IMPLEMENTATIONS
// ============================================================================

void ShipReportItem::report_stock(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.stock) return;

  const auto& s = *ship_;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(16);
  table.column(2).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(3).format().width(10).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(12).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(12).font_align(tabulate::FontAlign::center);
  table.column(6).format().width(14).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(12).font_align(tabulate::FontAlign::center);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row(
        {"#", "name", "x", "hanger", "res", "des", "fuel", "crew/mil"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number),
       std::format("{}{} {}", Shipltrs[s.type], s.crystals ? 'x' : ' ',
                   s.active ? s.name : "INACTIVE"),
       std::format("{}", s.crystals),
       std::format("{}:{}", s.hanger, s.max_hanger),
       std::format("{}:{}", s.resource, max_resource(s)),
       std::format("{}:{}", s.destruct, max_destruct(s)),
       std::format("{:.1f}:{}", s.fuel, max_fuel(s)),
       std::format("{}/{}:{}", s.popn, s.troops, s.max_crew)});

  g.out << table << "\n";
}

void ShipReportItem::report_status(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.status) return;

  const auto& s = *ship_;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(14);
  table.column(3).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(6).format().width(7).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(7).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(4).font_align(tabulate::FontAlign::right);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row({"#", "", "name", "las", "cew", "hyp", "guns", "arm", "tech",
                   "spd", "cost", "mass", "size"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Build special suffix for POD temperature
  std::string pod_suffix;
  if (s.type == ShipType::STYPE_POD) {
    if (std::holds_alternative<PodData>(s.special)) {
      auto pod = std::get<PodData>(s.special);
      pod_suffix = std::format(" ({})", pod.temperature);
    }
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.type]),
       std::format("{}", s.active ? s.name : "INACTIVE"), s.laser ? "yes" : "",
       s.cew ? "yes" : "", s.hyper_drive.has ? "yes" : "",
       std::format("{}{}/{}{}", s.primary, caliber_char(s.primtype),
                   s.secondary, caliber_char(s.sectype)),
       std::format("{}", armor(s)), std::format("{:.0f}", s.tech),
       std::format("{}", max_speed(s)), std::format("{}", shipcost(s)),
       std::format("{:.1f}", mass(s)),
       std::format("{}{}", size(s), pod_suffix)});

  g.out << table << "\n";
}

void ShipReportItem::report_weapons(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.weapons) return;

  const auto& s = *ship_;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(14);
  table.column(3).format().width(5).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(8).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(8).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(6).font_align(tabulate::FontAlign::center);
  table.column(8).format().width(8);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row(
        {"#", "", "name", "laser", "cew", "safe", "guns", "damage", "class"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Determine ship class string
  std::string ship_class;
  if ((s.type == ShipType::OTYPE_TERRA) || (s.type == ShipType::OTYPE_PLOW)) {
    ship_class = "Standard";
  } else {
    ship_class = s.shipclass;
  }

  // Add factory build type indicator if applicable
  std::string class_with_type;
  if (s.type == ShipType::OTYPE_FACTORY) {
    class_with_type =
        std::format("{} {}", Shipltrs[s.build_type], ship_class);
  } else {
    class_with_type = ship_class;
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.type]),
       std::format("{}", s.active ? s.name : "INACTIVE"), s.laser ? "yes" : "",
       std::format("{}/{}", s.cew, s.cew_range),
       std::format("{}", (int)((1.0 - .01 * s.damage) * s.tech / 4.0)),
       std::format("{}{}/{}{}", s.primary, caliber_char(s.primtype),
                   s.secondary, caliber_char(s.sectype)),
       std::format("{}%", s.damage), class_with_type});

  g.out << table << "\n";
}

void ShipReportItem::report_factories(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.factories || ship_->type != ShipType::OTYPE_FACTORY) return;

  const auto& s = *ship_;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(6).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(13).format().width(7).font_align(tabulate::FontAlign::center);
  table.column(14).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(15).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(16).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(17).format().width(4).font_align(tabulate::FontAlign::center);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row({"#", "", "Cost", "Tech", "Mass", "Sz", "A", "Crw", "Ful",
                   "Crg", "Hng", "Dst", "Sp", "Weapons", "Lsr", "CEWs",
                   "Range", "Dmg"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Handle special case for no ship type specified
  if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
    table.add_row({std::format("{}", s.number), "", "", "", "", "", "", "", "",
                   "", "", "", "", "(No ship type specified yet)", "", "", "",
                   "75% (OFF)"});
    g.out << table << "\n";
    return;
  }

  // Build weapon strings
  std::string prim_guns =
      s.primtype ? std::format("{}{}", s.primary, caliber_char(s.primtype))
                 : "---";

  std::string sec_guns =
      s.sectype ? std::format("{}{}", s.secondary, caliber_char(s.sectype))
                : "---";

  std::string cew_str = s.cew ? std::format("{}", s.cew) : "----";
  std::string range_str = s.cew ? std::format("{}", s.cew_range) : "-----";

  // Build speed indicator (hyper + mount + speed)
  std::string speed_indicator;
  if (s.hyper_drive.has) {
    speed_indicator = s.mount ? "+" : "*";
  } else {
    speed_indicator = " ";
  }
  speed_indicator += std::format("{}", s.max_speed);

  // Build damage status
  std::string damage_status = std::format("{}%", s.damage);
  if (s.damage) {
    if (!s.on) damage_status += "*";
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.build_type]),
       std::format("{}", s.build_cost), std::format("{:.1f}", s.complexity),
       std::format("{:.1f}", s.base_mass), std::format("{}", ship_size(s)),
       std::format("{}", s.armor), std::format("{}", s.max_crew),
       std::format("{}", s.max_fuel), std::format("{}", s.max_resource),
       std::format("{}", s.max_hanger), std::format("{}", s.max_destruct),
       speed_indicator, std::format("{}/{}", prim_guns, sec_guns),
       s.laser ? "yes" : " no", cew_str, range_str, damage_status});

  g.out << table << "\n";
}

void ShipReportItem::report_general(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.report) return;

  const auto& s = *ship_;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(1).format().width(5);
  table.column(2).format().width(12);
  table.column(3).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(2).font_align(tabulate::FontAlign::center);
  table.column(10).format().width(10);
  table.column(11).format().width(18);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row({"", "#", "name", "gov", "dam", "crew", "mil", "des", "fuel",
                   "sp", "orbits", "destination"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Build location/destination string
  std::string locstrn;
  if (s.docked) {
    if (s.whatdest == ScopeLevel::LEVEL_SHIP)
      locstrn = std::format("D#{}", s.destshipno);
    else
      locstrn = std::format("L{:2},{:<2}", s.land_x, s.land_y);
  } else if (s.navigate.on) {
    locstrn = std::format("nav:{} ({})", s.navigate.bearing, s.navigate.turns);
  } else {
    locstrn = prin_ship_dest(s);
  }

  // Build name string (may have special formatting for inactive ships)
  std::string name_str;
  if (!s.active) {
    name_str = std::format("INACTIVE({})", s.rad);
  } else {
    name_str = s.name;
  }

  // Build speed indicator
  std::string speed_indicator;
  if (s.hyper_drive.has) {
    speed_indicator = s.mounted ? "+" : "*";
  } else {
    speed_indicator = " ";
  }
  speed_indicator += std::format("{}", s.speed);

  // Add data row
  table.add_row({std::format("{}", Shipltrs[s.type]),
                 std::format("{}", s.number), name_str,
                 std::format("{}", s.governor), std::format("{}", s.damage),
                 std::format("{}", s.popn), std::format("{}", s.troops),
                 std::format("{}", s.destruct), std::format("{:.0f}", s.fuel),
                 speed_indicator, dispshiploc_brief(s), locstrn});

  g.out << table << "\n";
}

bool ShipReportItem::should_report(player_t player_num, governor_t governor,
                                   const ReportSet& rep_on) const {
  const auto& s = *ship_;

  // Don't report on ships that are dead
  if (!s.alive) return false;

  // Don't report on ships not owned by this player
  if (s.owner != player_num) return false;

  // Don't report on ships this governor is not authorized for
  if (!authorized(governor, s)) return false;

  // Don't report on ships whose type isn't in the requested report filter
  if (!rep_on.contains(Shipltrs[s.type])) return false;

  // Don't report on undocked canisters (launched canisters don't show up)
  if (s.type == ShipType::OTYPE_CANIST && !s.docked) return false;

  // Don't report on undocked greens (launched greens don't show up)
  if (s.type == ShipType::OTYPE_GREEN && !s.docked) return false;

  return true;
}

// ============================================================================
// PLANET REPORT METHOD IMPLEMENTATIONS
// ============================================================================

bool PlanetReportItem::should_report(player_t player_num, governor_t,
                                     const ReportSet&) const {
  // Don't report on planets where we don't own any sectors
  return planet_->info[player_num - 1].numsectsowned != 0;
}

// ============================================================================
// TACTICAL REPORT IMPLEMENTATIONS
// ============================================================================

void ShipReportItem::add_tactical_header_row(
    tabulate::Table& table, GameObj&, player_t,
    const TacticalParams& params) const {
  const auto& s = *ship_;
  Place where{s.whatorbits, s.storbits, s.pnumorbits};

  std::string name_str = s.active ? s.name : "INACTIVE";
  std::string orbits_str = where.to_string();

  // Build landed location suffix
  std::string location_suffix;
  if (landed(s)) {
    location_suffix = std::format(" ({},{})", s.land_x, s.land_y);
  }

  // Build inactive suffix
  std::string inactive_suffix;
  if (!s.active) {
    inactive_suffix = std::format(" INACTIVE({})", s.rad);
  }

  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.type]),
       name_str, std::format("{:.0f}", s.tech),
       std::format("{}{}/{}{}", s.primary, caliber_char(s.primtype),
                   s.secondary, caliber_char(s.sectype)),
       std::format("{}", s.armor), std::format("{}", s.size),
       std::format("{}", s.destruct), std::format("{:.1f}", s.fuel),
       std::format("{}%", s.damage), std::format("{}", params.fspeed),
       params.fev ? "yes" : "",
       std::format("{}{}{}", orbits_str, location_suffix, inactive_suffix)});
}

void ShipReportItem::add_tactical_target_row(
    tabulate::Table& table, GameObj& g, RstContext& ctx, const Race& race,
    double dist, const FiringShipParams& firer) const {
  const auto& s = *ship_;

  // Filter ships based on player filter or ship type list
  // If filter_player is set, only show ships owned by that player
  // If shiplist is non-empty, only show ships whose type is in the list
  if (ctx.filter_player.has_value()) {
    // Player filter mode: show only ships owned by specified player
    if (s.owner != *ctx.filter_player) {
      return;
    }
  } else if (!ctx.shiplist.empty()) {
    // Ship type filter mode: show only ships whose type is in the list
    if (!listed(s.type, ctx.shiplist)) {
      return;
    }
  }

  // Don't show ships we own and are authorized for
  if (s.owner == g.player && authorized(g.governor, s)) {
    return;
  }

  // Don't show dead ships
  if (!s.alive) {
    return;
  }

  // Don't show canisters or greenhouse gases
  if (s.type == ShipType::OTYPE_CANIST || s.type == ShipType::OTYPE_GREEN) {
    return;
  }

  // Calculate target ship's evasion and speed (only if moving and active)
  bool tev = false;
  int tspeed = 0;
  if ((s.whatdest != ScopeLevel::LEVEL_UNIV || s.navigate.on) && !s.docked &&
      s.active) {
    tspeed = s.speed;
    tev = s.protect.evade;
  }

  // Calculate combat parameters using firer's data and target's data
  int body = size(s);
  auto defense = getdefense(s);
  auto [prob, factor] =
      hit_odds(dist, firer.tech, firer.damage, firer.evade, tev, firer.speed,
               tspeed, body, firer.caliber, defense);

  // Apply laser focus bonus if firer has it
  if (firer.laser_focused) {
    prob = prob * prob / 100;
  }

  // Determine diplomatic status indicator
  const auto* war_status = isset(race.atwar, s.owner)    ? "-"
                           : isset(race.allied, s.owner) ? "+"
                                                         : " ";

  // Filter out allied ships if enemies-only mode is enabled
  if (ctx.enemies_only && isset(race.allied, s.owner)) {
    return;
  }

  // Build location string
  std::string loc_str;
  if (landed(s)) {
    loc_str = std::format("({},{})", s.land_x, s.land_y);
  }

  // Build status suffix
  std::string status_suffix = s.active ? "" : " INACTIVE";

  // Add row to table
  table.add_row({std::format("{}", s.number),
                 std::format("{}{},{}", war_status, s.owner, s.governor),
                 std::format("{}", Shipltrs[s.type]),
                 std::format("{:.14}", s.name), std::format("{:.0f}", dist),
                 std::format("{}", factor), std::format("{}", body),
                 std::format("{}", tspeed), tev ? "yes" : "",
                 std::format("{}%", prob), std::format("{}%", s.damage),
                 std::format("{}{}", loc_str, status_suffix)});
}

TacticalParams ShipReportItem::get_tactical_params(const Race&) const {
  TacticalParams params{};
  const auto& s = *ship_;
  params.tech = s.tech;

  if ((s.whatdest != ScopeLevel::LEVEL_UNIV || s.navigate.on) && !s.docked &&
      s.active) {
    params.fspeed = s.speed;
    params.fev = s.protect.evade;
  }

  return params;
}

std::optional<starnum_t> ShipReportItem::get_star_orbit() const {
  if (ship_->whatorbits != ScopeLevel::LEVEL_UNIV) {
    return ship_->storbits;
  }
  return std::nullopt;
}

void PlanetReportItem::add_tactical_header_row(
    tabulate::Table& table, GameObj& g, player_t player_num,
    const TacticalParams& params) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id);

  std::string name_str = std::format(
      "(planet){}", star ? star->pnames[p.planet_order] : "Unknown");

  table.add_row({"", "", name_str, std::format("{:.0f}", params.tech),
                 std::format("{}M", p.info[player_num - 1].guns), "", "",
                 std::format("{}", p.info[player_num - 1].destruct),
                 std::format("{}", p.info[player_num - 1].fuel), "", "", "",
                 ""});
}

void PlanetReportItem::add_tactical_target_row(tabulate::Table& table,
                                               GameObj& g, RstContext&,
                                               const Race&, double dist,
                                               const FiringShipParams&) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(p.star_id);
  std::string name_str = star ? star->pnames[p.planet_order] : "Unknown";

  table.add_row({"", "(planet)", "", name_str, std::format("{:.0f}", dist), "",
                 "", "", "", "", "", ""});
}

TacticalParams PlanetReportItem::get_tactical_params(const Race& race) const {
  TacticalParams params{};
  params.tech = race.tech;
  // Planets don't have speed or evasion
  return params;
}

void ShipReportItem::report_tactical(GameObj& g, RstContext& ctx,
                                     const TacticalParams& params) const {
  if (!ctx.flags.tactical) return;

  const auto* race = g.entity_manager.peek_race(g.player);
  if (!race) return;

  const auto& s = *ship_;

  bool sight = shipsight(s);
  if (!sight) return;

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
  add_tactical_header_row(header_table, g, g.player, params);

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

  for (const auto& target : ctx.rd) {
    // Skip ourselves
    if (target->x() == x_ && target->y() == y_) continue;

    double range = gun_range(s);
    double dist = sqrt(Distsq(x_, y_, target->x(), target->y()));
    if (dist >= range) continue;

    // Build firing ship parameters
    FiringShipParams firer{
        .tech = params.tech,
        .damage = s.damage,
        .evade = params.fev,
        .speed = params.fspeed,
        .caliber = current_caliber(s),
        .laser_focused = (laser_on(s) && s.focus),
    };

    // Polymorphic call - adds row to table for ships, skips for planets not in
    // range
    target->add_tactical_target_row(tactical_table, g, ctx, *race, dist, firer);
  }

  // Only output tactical table if we have targets
  if (tactical_table.size() > 1) {  // More than just the header
    g.out << "\n" << tactical_table << "\n";
  }
}

void PlanetReportItem::report_tactical(GameObj& g, RstContext& ctx,
                                       const TacticalParams& params) const {
  if (!ctx.flags.tactical) return;

  const auto* race = g.entity_manager.peek_race(g.player);
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
  add_tactical_header_row(header_table, g, g.player, params);

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

  for (shipnum_t i = 0; i < ctx.rd.size(); i++) {
    // Skip ourselves
    if (ctx.rd[i]->x() == x_ && ctx.rd[i]->y() == y_) continue;

    double range = gun_range(*race);
    double dist = sqrt(Distsq(x_, y_, ctx.rd[i]->x(), ctx.rd[i]->y()));
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
    ctx.rd[i]->add_tactical_target_row(tactical_table, g, ctx, *race, dist,
                                       firer);
  }

  // Only output tactical table if we have targets
  if (tactical_table.size() > 1) {  // More than just the header
    g.out << "\n" << tactical_table << "\n";
  }
}

// ============================================================================
// MAIN REPORT DRIVER
// ============================================================================

void ship_report(GameObj& g, RstContext& ctx, const ReportItem& item,
                 const ReportSet& rep_on) {
  // Get race from EntityManager
  const auto* race = g.entity_manager.peek_race(g.player);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  // Check if this item should be reported
  if (!item.should_report(g.player, g.governor, rep_on)) {
    return;
  }

  // Calculate tactical parameters for report_tactical (polymorphic)
  TacticalParams params = item.get_tactical_params(*race);

  // Polymorphic dispatch to appropriate report methods
  item.report_stock(g, ctx);
  item.report_status(g, ctx);
  item.report_weapons(g, ctx);
  item.report_factories(g, ctx);
  item.report_general(g, ctx);
  item.report_tactical(g, ctx, params);
}
}  // namespace

// ============================================================================
// COMMAND ENTRY POINT
// ============================================================================

namespace GB::commands {
void rst(const command_t& argv, GameObj& g) {
  ReportSet report_types;

  RstContext ctx;
  ctx.enemies_only = false;
  ctx.first = true;

  // Set report flags based on command name
  for (const auto& [cmd, flags] : kCommandReportModes) {
    if (argv[0] == cmd) {
      ctx.flags = flags;
      break;
    }
  }

  shipnum_t n_ships = Numships();

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

  if (argv.size() >= 2) {
    if (*argv[1].c_str() == '#' || isdigit(*argv[1].c_str())) {
      /* report on a couple ships */
      int l = 1;
      while (l < MAXARGS && *argv[l].c_str() != '\0') {
        shipnum_t shipno;
        sscanf(argv[l].c_str() + (*argv[l].c_str() == '#'), "%lu", &shipno);
        if (shipno > n_ships || shipno < 1) {
          g.out << std::format("rst: no such ship #{} \n", shipno);
          return;
        }
        get_report_ship(g, ctx, shipno);
        // Check if ship is in a star system and load those ships too
        if (auto star_opt = ctx.rd.back()->get_star_orbit()) {
          star_get_report_ships(g, ctx, g.player, *star_opt);
          ship_report(g, ctx, *ctx.rd.back(), report_types);
        } else {
          ship_report(g, ctx, *ctx.rd.back(), report_types);
        }
        l++;
      }
      return;
    }

    // Parse ship type letters and add to set - only valid ship letters
    std::ranges::copy_if(
        argv[1], std::inserter(report_types, report_types.end()),
        [](char c) { return std::ranges::contains(Shipltrs, c); });

    // Warn if no valid ship types were found
    if (report_types.empty()) {
      g.out << std::format("'{}' -- no valid ship letters found\n", argv[1]);
      return;
    }
  } else {
    // No ship type filter specified - report all types
    std::ranges::copy(Shipltrs,
                      std::inserter(report_types, report_types.end()));
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (!ctx.flags.tactical || argv.size() >= 2) {
        shipnum_t shn = Sdata.ships;
        while (shn && get_report_ship(g, ctx, shn)) {
          shn = ctx.rd.back()->next_ship();
        }

        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_get_report_ships(g, ctx, g.player, i);
        for (const auto& item : ctx.rd)
          ship_report(g, ctx, *item, report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_get_report_ships(g, ctx, g.player, g.snum, g.pnum);
      for (const auto& item : ctx.rd) ship_report(g, ctx, *item, report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_get_report_ships(g, ctx, g.player, g.snum);
      for (const auto& item : ctx.rd) ship_report(g, ctx, *item, report_types);
      break;
    case ScopeLevel::LEVEL_SHIP:
      if (g.shipno == 0) {
        g.out << "Error: No ship is currently scoped. Use 'cs #<shipno>' to "
                 "scope to a ship.\n";
        return;
      }
      get_report_ship(g, ctx, g.shipno);
      if (ctx.rd.empty()) {
        g.out << std::format("Error: Unable to retrieve ship #{} data.\n",
                             g.shipno);
        return;
      }
      ship_report(g, ctx, *ctx.rd[0], report_types); /* first ship report */

      // Report on ships docked in this ship
      RstContext docked_ctx{
          .rd = {},  // Empty vector for docked ships
          .shiplist = ctx.shiplist,
          .flags = ctx.flags,
          .first = ctx.first,
          .enemies_only = ctx.enemies_only,
          .filter_player = ctx.filter_player,
      };

      shipnum_t shn = ctx.rd[0]->child_ships();
      while (shn && get_report_ship(g, docked_ctx, shn)) {
        shn = docked_ctx.rd.back()->next_ship();
      }
      for (const auto& item : docked_ctx.rd) {
        ship_report(g, docked_ctx, *item, report_types);
      }
      break;
  }
}
}  // namespace GB::commands