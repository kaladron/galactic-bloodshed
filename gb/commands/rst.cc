// SPDX-License-Identifier: Apache-2.0

/// \file rst.cc
/// \brief Ship and planet reporting commands (report, stock, tactical, stats,
///        weapons, factories)
///
/// Implements various reporting modes for ships and planets including status,
/// stock levels, tactical combat information, and factory configurations.
///
/// This file provides a polymorphic reporting system for ships and planets,
/// allowing multiple report types (status, stock, tactical, weapons, etc.)
/// to be generated based on command-line options. The system uses virtual
/// dispatch through the ReportItem base class.

module;

import gblib;
import std.compat;

module commands;

namespace {
// ============================================================================
// Constants and Type Definitions
// ============================================================================

/// Gun caliber display characters for output formatting
constexpr char kCaliber[] = {' ', 'L', 'M', 'H'};

/// Array indicating which ship types to include in the report
using ReportArray = std::array<bool, NUMSTYPES>;

// Forward declarations
struct RstContext;

/// Tactical combat parameters used for calculating hit probabilities
/// and displaying combat-relevant information in tactical reports.
struct TacticalParams {
  double tech = 0.0;   ///< Technology level of firing entity
  bool fev = false;    ///< Whether firing entity is evading
  int fspeed = 0;      ///< Speed of firing entity
};

// ============================================================================
// ReportItem Class Hierarchy
// ============================================================================

/// Abstract base class for report items (ships and planets).
///
/// This class provides a polymorphic interface for generating various types
/// of reports for both ships and planets. Different report types (status,
/// stock, tactical, weapons, factories) are implemented as virtual methods
/// that can be overridden by derived classes.
///
/// The class uses the Template Method pattern where ship_report() calls
/// these virtual methods in sequence based on active report flags.
class ReportItem {
 protected:
  double x_;  ///< X coordinate in space
  double y_;  ///< Y coordinate in space

 public:
  ReportItem(double x, double y) : x_(x), y_(y) {}
  virtual ~ReportItem() = default;

  /// Get X coordinate of this item
  double x() const { return x_; }
  
  /// Get Y coordinate of this item
  double y() const { return y_; }

  // Report generation methods - default implementations do nothing
  // (overridden by derived classes as needed)
  
  /// Generate stock report (resources, fuel, crew, etc.)
  virtual void report_stock([[maybe_unused]] GameObj& g,
                            [[maybe_unused]] RstContext& ctx) const {}
  
  /// Generate status report (technology, weapons, armor, etc.)
  virtual void report_status([[maybe_unused]] GameObj& g,
                             [[maybe_unused]] RstContext& ctx) const {}
  
  /// Generate weapons report (laser, CEW, guns, damage, etc.)
  virtual void report_weapons([[maybe_unused]] GameObj& g,
                              [[maybe_unused]] RstContext& ctx) const {}
  
  /// Generate factory report (ship being built, costs, etc.)
  virtual void report_factories([[maybe_unused]] GameObj& g,
                                [[maybe_unused]] RstContext& ctx) const {}
  
  /// Generate general status report (orbits, destination, damage, crew)
  virtual void report_general([[maybe_unused]] GameObj& g,
                              [[maybe_unused]] RstContext& ctx) const {}
  
  /// Generate tactical combat report showing nearby targets and hit probabilities
  virtual void report_tactical(
      [[maybe_unused]] GameObj& g, [[maybe_unused]] RstContext& ctx,
      [[maybe_unused]] const TacticalParams& params) const {}

  /// Display this item as a tactical target in another item's tactical report.
  /// Shows distance, hit probability, evasion, and other combat-relevant data.
  virtual void print_tactical_target(
      [[maybe_unused]] GameObj& g, [[maybe_unused]] RstContext& ctx,
      [[maybe_unused]] const Race& race, [[maybe_unused]] shipnum_t indx,
      [[maybe_unused]] double dist, [[maybe_unused]] double tech,
      [[maybe_unused]] int fdam, [[maybe_unused]] bool fev,
      [[maybe_unused]] int fspeed, [[maybe_unused]] guntype_t caliber) const {}

  /// Apply laser focus bonus to hit probability.
  /// Ships with active lasers and focus enabled get squared probability.
  /// Default implementation returns the probability unchanged (for planets).
  virtual int apply_laser_focus(int prob) const { return prob; }

  /// Print the header summary line for tactical reports.
  /// Shows the firing entity's stats (tech, guns, armor, etc.)
  virtual void print_tactical_header_summary(
      [[maybe_unused]] GameObj& g, [[maybe_unused]] player_t player_num,
      [[maybe_unused]] const TacticalParams& params) const {}

  /// Get tactical parameters for this item (tech, speed, evasion)
  virtual TacticalParams get_tactical_params(
      [[maybe_unused]] const Race& race) const {
    return TacticalParams{};
  }

  /// Get star number if this item is in a star system.
  /// Returns nullopt for items in deep space or not applicable.
  virtual std::optional<starnum_t> get_star_orbit() const {
    return std::nullopt;
  }

  /// Get next ship number in linked list (ships only).
  /// Returns 0 for planets or end of list.
  virtual shipnum_t next_ship() const { return 0; }

  /// Get first child ship docked/landed on this item (ships only).
  /// Returns 0 for planets or no children.
  virtual shipnum_t child_ships() const { return 0; }

  /// Check if this item should be included in the report.
  /// Filters by owner, governor authorization, ship type, etc.
  virtual bool should_report(player_t player_num, governor_t governor,
                             const ReportArray& rep_on) const = 0;
};

/// Check if this item should be included in the report.
  /// Filters by owner, governor authorization, ship type, etc.
  virtual bool should_report(player_t player_num, governor_t governor,
                             const ReportArray& rep_on) const = 0;
};

/// Ship report item - holds non-owning pointer from peek_ship.
///
/// Represents a ship in the report system. This class wraps a ship pointer
/// obtained from the EntityManager and provides all ship-specific reporting
/// functionality including stock, status, weapons, factories, and tactical
/// reports.
class ShipReportItem : public ReportItem {
  shipnum_t n_;      ///< Ship number
  const Ship* ship_; ///< Non-owning pointer to ship data

 public:
  ShipReportItem(shipnum_t n, const Ship* ship)
      : ReportItem(ship->xpos, ship->ypos), n_(n), ship_(ship) {}

  /// Get ship number
  shipnum_t shipno() const { return n_; }
  
  /// Get reference to ship data
  const Ship& ship() const { return *ship_; }

  void report_stock(GameObj& g, RstContext& ctx) const override;
  void report_status(GameObj& g, RstContext& ctx) const override;
  void report_weapons(GameObj& g, RstContext& ctx) const override;
  void report_factories(GameObj& g, RstContext& ctx) const override;
  void report_general(GameObj& g, RstContext& ctx) const override;
  void report_tactical(GameObj& g, RstContext& ctx,
                       const TacticalParams& params) const override;

  void print_tactical_target(GameObj& g, RstContext& ctx, const Race& race,
                             shipnum_t indx, double dist, double tech, int fdam,
                             bool fev, int fspeed,
                             guntype_t caliber) const override;

  int apply_laser_focus(int prob) const override;

  void print_tactical_header_summary(
      GameObj& g, player_t player_num,
      const TacticalParams& params) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  /// Get star number if ship is orbiting a star system
  std::optional<starnum_t> get_star_orbit() const override {
    if (ship_->whatorbits != ScopeLevel::LEVEL_UNIV) {
      return ship_->storbits;
    }
    return std::nullopt;
  }

  /// Get next ship number in the linked list
  shipnum_t next_ship() const override { return ship_->nextship; }

  /// Get first child ship docked/landed on this ship
  shipnum_t child_ships() const override { return ship_->ships; }

  bool should_report(player_t player_num, governor_t governor,
                     const ReportArray& rep_on) const override;
};

  bool should_report(player_t player_num, governor_t governor,
                     const ReportArray& rep_on) const override;
};

/// Planet report item - holds non-owning pointer from peek_planet.
///
/// Represents a planet in the report system. Planets primarily appear in
/// tactical reports to show planetary defenses and as potential targets.
/// Unlike ships, planets don't have stock, status, weapons, or factory reports.
class PlanetReportItem : public ReportItem {
  starnum_t star_;       ///< Star system number
  planetnum_t pnum_;     ///< Planet number within star system
  const Planet* planet_; ///< Non-owning pointer to planet data

 public:
  PlanetReportItem(starnum_t star, planetnum_t pnum, const Planet* planet,
                   double x, double y)
      : ReportItem(x, y), star_(star), pnum_(pnum), planet_(planet) {}

  /// Get star system number
  starnum_t star() const { return star_; }
  
  /// Get planet number within star system
  planetnum_t pnum() const { return pnum_; }
  
  /// Get reference to planet data
  const Planet& planet() const { return *planet_; }

  void report_tactical(GameObj& g, RstContext& ctx,
                       const TacticalParams& params) const override;

  void print_tactical_target(GameObj& g, RstContext& ctx, const Race& race,
                             shipnum_t indx, double dist, double tech, int fdam,
                             bool fev, int fspeed,
                             guntype_t caliber) const override;

  void print_tactical_header_summary(
      GameObj& g, player_t player_num,
      const TacticalParams& params) const override;

  TacticalParams get_tactical_params(const Race& race) const override;

  bool should_report(player_t player_num, governor_t governor,
                     const ReportArray& rep_on) const override;
};

// ============================================================================
// Report Configuration Structures
// ============================================================================

/// Report mode flags - all default to false.
/// These flags control which report types are active for a given command.
struct ReportFlags {
  bool status = false;     ///< Show status report (tech, guns, armor, etc.)
  bool ship = false;       ///< Show comprehensive ship report (all types)
  bool stock = false;      ///< Show stock report (resources, fuel, crew)
  bool report = false;     ///< Show general report (orbits, destination)
  bool weapons = false;    ///< Show weapons report (laser, CEW, guns)
  bool factories = false;  ///< Show factory report (ship being built)
  bool tactical = false;   ///< Show tactical combat report
};

/// Context structure holding all rst command state.
/// This structure is passed through the entire reporting pipeline and
/// maintains state about what items to report, which flags are active,
/// and formatting state like whether headers have been printed.
struct RstContext {
  std::vector<std::unique_ptr<ReportItem>> rd; ///< Collection of items to report
  std::string shiplist;      ///< Ship type filter string (e.g., "dsc" for destroyers/scouts/cruisers)
  ReportFlags flags;         ///< Active report modes
  bool first;                ///< True if we haven't printed headers yet
  bool enemies_only;         ///< True to filter out allied ships in tactical
  int who;                   ///< Player filter (0=all, player#, or 999=shiplist filter)
};

/// Map of command names to their report flag configurations.
/// Defines which report types are enabled for each command variant.
/// TODO(jeffbailey): Replace with std::unordered_map when available in C++20 modules
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
// Helper Functions - Ship/Planet Collection
// ============================================================================

/// Check if a ship type letter appears in the filter string.
///
/// \param type Ship type index
/// \param string Filter string containing ship type letters
/// \return true if the ship's letter is in the filter string
bool listed(int type, const std::string& string) {
  return std::ranges::any_of(string,
                             [type](char c) { return Shipltrs[type] == c; });
}

/// Load a ship from the entity manager and add it to the report list.
///
/// \param g Game context
/// \param ctx Report context to add ship to
/// \param shipno Ship number to load
/// \return true if ship was successfully loaded, false otherwise
bool get_report_ship(GameObj& g, RstContext& ctx, shipnum_t shipno) {
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (ship) {
    ctx.rd.push_back(std::make_unique<ShipReportItem>(shipno, ship));
    return true;
  }
  g.out << std::format("get_report_ship: error on ship get ({}).\n", shipno);
  return false;
}

/// Collect all ships at a planet and the planet itself for reporting.
///
/// Adds the planet to the report list, then walks the linked list of ships
/// at that planet, adding each explored ship to the report list.
///
/// \param g Game context
/// \param ctx Report context to add items to
/// \param player_num Player number for exploration check
/// \param snum Star system number
/// \param pnum Planet number within star system
void plan_get_report_ships(GameObj& g, RstContext& ctx, player_t player_num,
                           starnum_t snum, planetnum_t pnum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  const auto* planet = g.entity_manager.peek_planet(snum, pnum);
  if (!planet) return;

  // Add planet to report list
  double x = star->xpos + planet->xpos;
  double y = star->ypos + planet->ypos;
  ctx.rd.push_back(
      std::make_unique<PlanetReportItem>(snum, pnum, planet, x, y));

  // Add ships at this planet if explored
  if (planet->info[player_num - 1].explored) {
    shipnum_t shn = planet->ships;
    while (shn && get_report_ship(g, ctx, shn)) {
      shn = ctx.rd.back()->next_ship();
    }
  }
}

/// Collect all ships in a star system and all planets for reporting.
///
/// Walks the ship list at star level, then iterates through all planets
/// in the star system, collecting ships at each planet.
///
/// \param g Game context
/// \param ctx Report context to add items to
/// \param player_num Player number for exploration check
/// \param snum Star system number
void star_get_report_ships(GameObj& g, RstContext& ctx, player_t player_num,
                           starnum_t snum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (isset(star->explored, player_num)) {
    // Add ships orbiting the star
    shipnum_t shn = star->ships;
    while (shn && get_report_ship(g, ctx, shn)) {
      shn = ctx.rd.back()->next_ship();
    }
    // Add planets and their ships
    for (planetnum_t i = 0; i < star->numplanets; i++)
      plan_get_report_ships(g, ctx, player_num, snum, i);
  }
}

// ============================================================================
// ShipReportItem Method Implementations
// ============================================================================

/// Generate stock report for a ship.
///
/// Shows resources, fuel, crew, cargo capacity, and other inventory items.
/// Format: # type name crystals hanger resources destruct fuel crew/troops
/// Generate stock report for a ship.
///
/// Shows resources, fuel, crew, cargo capacity, and other inventory items.
/// Format: # type name crystals hanger resources destruct fuel crew/troops
void ShipReportItem::report_stock(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.stock) return;

  if (ctx.first) {
    g.out << "    #       name        x  hanger   res        des       "
             "  fuel      crew/mil\n";
    if (!ctx.flags.ship) ctx.first = false;
  }
  const auto& s = *ship_;
  g.out << std::format(
      "{:5} {:c} "
      "{:14.14}{:3}{:4}:{:<3}{:5}:{:<5}{:5}:{:<5}{:7.1f}:{:<6}{}/{}:{}\n",
      n_, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"), s.crystals,
      s.hanger, s.max_hanger, s.resource, max_resource(s), s.destruct,
      max_destruct(s), s.fuel, max_fuel(s), s.popn, s.troops, s.max_crew);
}

/// Generate status report for a ship.
///
/// Shows technology, weapons systems, armor, speed, and other ship stats.
/// Format: # type name laser cew hyper guns armor tech speed cost mass size
/// Generate status report for a ship.
///
/// Shows technology, weapons systems, armor, speed, and other ship stats.
/// Format: # type name laser cew hyper guns armor tech speed cost mass size
void ShipReportItem::report_status(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.status) return;

  if (ctx.first) {
    g.out << "    #       name       las cew hyp    guns   arm tech "
             "spd cost  mass size\n";
    if (!ctx.flags.ship) ctx.first = false;
  }
  const auto& s = *ship_;
  g.out << std::format(
      "{:5} {:c} {:14.14} "
      "{}{}{}{:3}{:c}/{:3}{:c}{:4}{:5.0f}{:4}{:5}{:7.1f}{:4}",
      n_, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
      s.laser ? "yes " : "    ", s.cew ? "yes " : "    ",
      s.hyper_drive.has ? "yes " : "    ", s.primary, kCaliber[s.primtype],
      s.secondary, kCaliber[s.sectype], armor(s), s.tech, max_speed(s),
      shipcost(s), mass(s), size(s));
  if (s.type == ShipType::STYPE_POD) {
    if (std::holds_alternative<PodData>(s.special)) {
      auto pod = std::get<PodData>(s.special);
      g.out << std::format(" ({})", pod.temperature);
    }
  }
  g.out << "\n";
}

/// Generate weapons report for a ship.
///
/// Shows combat-related information: laser, CEW, guns, damage, and class.
/// Format: # type name laser cew guns damage class
/// Generate weapons report for a ship.
///
/// Shows combat-related information: laser, CEW, guns, damage, and class.
/// Format: # type name laser cew guns damage class
void ShipReportItem::report_weapons(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.weapons) return;

  if (ctx.first) {
    g.out << "    #       name      laser   cew     safe     guns    "
             "damage   class\n";
    if (!ctx.flags.ship) ctx.first = false;
  }
  const auto& s = *ship_;
  g.out << std::format(
      "{:5} {:c} {:14.14} {}  {:3}/{:<4}  {:4}  {:3}{:c}/{:3}{:c}    {:3}% "
      " {:c} {}\n",
      n_, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
      s.laser ? "yes " : "    ", s.cew, s.cew_range,
      (int)((1.0 - .01 * s.damage) * s.tech / 4.0), s.primary,
      kCaliber[s.primtype], s.secondary, kCaliber[s.sectype], s.damage,
      s.type == ShipType::OTYPE_FACTORY ? Shipltrs[s.build_type] : ' ',
      ((s.type == ShipType::OTYPE_TERRA) || (s.type == ShipType::OTYPE_PLOW))
          ? "Standard"
          : s.shipclass);
}

/// Generate factory report for a ship.
///
/// Shows the configuration of ships being built by factory ships, including
/// cost, tech level, mass, size, armor, weapons, and other stats.
/// Only displays for factory-type ships that have a build configuration.
/// Generate factory report for a ship.
///
/// Shows the configuration of ships being built by factory ships, including
/// cost, tech level, mass, size, armor, weapons, and other stats.
/// Only displays for factory-type ships that have a build configuration.
void ShipReportItem::report_factories(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.factories || ship_->type != ShipType::OTYPE_FACTORY) return;

  if (ctx.first) {
    g.out << "   #    Cost Tech Mass Sz A Crw Ful Crg Hng Dst Sp "
             "Weapons Lsr CEWs Range Dmg\n";
    if (!ctx.flags.ship) ctx.first = false;
  }

  const auto& s = *ship_;

  if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
    g.out << std::format(
        "{:5}               (No ship type specified yet)           "
        "           75% (OFF)",
        n_);
    return;
  }

  std::string tmpbuf1;
  if (s.primtype) {
    const char* caliber;
    switch (s.primtype) {
      case GTYPE_LIGHT:
        caliber = "L";
        break;
      case GTYPE_MEDIUM:
        caliber = "M";
        break;
      case GTYPE_HEAVY:
        caliber = "H";
        break;
      default:
        caliber = "N";
        break;
    }
    tmpbuf1 = std::format("{:2}{}", s.primary, caliber);
  } else {
    tmpbuf1 = "---";
  }

  std::string tmpbuf2;
  if (s.sectype) {
    const char* caliber;
    switch (s.sectype) {
      case GTYPE_LIGHT:
        caliber = "L";
        break;
      case GTYPE_MEDIUM:
        caliber = "M";
        break;
      case GTYPE_HEAVY:
        caliber = "H";
        break;
      default:
        caliber = "N";
        break;
    }
    tmpbuf2 = std::format("{:2}{}", s.secondary, caliber);
  } else {
    tmpbuf2 = "---";
  }

  std::string tmpbuf3 = s.cew ? std::format("{:4}", s.cew) : "----";
  std::string tmpbuf4 = s.cew ? std::format("{:5}", s.cew_range) : "-----";

  g.out << std::format(
      "{:5} {:c}{:4}{:6.1f}{:5.1f}{:3}{:2}{:4}{:4}{:4}{:4}{:4} {}{:1} "
      "{}/{} {} "
      "{} {} {:02}%{}\n",
      n_, Shipltrs[s.build_type], s.build_cost, s.complexity, s.base_mass,
      ship_size(s), s.armor, s.max_crew, s.max_fuel, s.max_resource,
      s.max_hanger, s.max_destruct,
      s.hyper_drive.has ? (s.mount ? "+" : "*") : " ", s.max_speed, tmpbuf1,
      tmpbuf2, s.laser ? "yes" : " no", tmpbuf3, tmpbuf4, s.damage,
      s.damage ? (s.on ? "" : "*") : "");
}

/// Generate general report for a ship.
///
/// Shows ship location, destination, governor, damage, crew, and fuel status.
/// This is the main overview report showing where ships are and where they're going.
void ShipReportItem::report_general(GameObj& g, RstContext& ctx) const {
  if (!ctx.flags.report) return;

  if (ctx.first) {
    g.out << " #      name       gov dam crew mil  des fuel sp orbits  "
             "   destination\n";
    if (!ctx.flags.ship) ctx.first = false;
  }

  const auto& s = *ship_;

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

  std::string strng;
  if (!s.active) {
    strng = std::format("INACTIVE({})", s.rad);
  }

  g.out << std::format(
      "{:c}{:<5} {:12.12} {:2} {:3}{:5}{:4}{:5}{:5.0f} {:c}{:1} {:<10} "
      "{:<18}\n",
      Shipltrs[s.type], n_, (s.active ? s.name : strng), s.governor, s.damage,
      s.popn, s.troops, s.destruct, s.fuel,
      s.hyper_drive.has ? (s.mounted ? '+' : '*') : ' ', s.speed,
      dispshiploc_brief(s), locstrn);
}

/// Check if a ship should be included in the report.
///
/// Filters ships based on:
/// - Ship must be alive
/// - Must be owned by the reporting player
/// - Governor must be authorized for this ship
/// - Ship type must be in the requested filter
/// - Special rules for canisters and greenhouse gases (only show when docked)
///
/// \return true if ship should be included in report
bool ShipReportItem::should_report(player_t player_num, governor_t governor,
                                   const ReportArray& rep_on) const {
  const auto& s = *ship_;

  // Don't report on ships that are dead
  if (!s.alive) return false;

  // Don't report on ships not owned by this player
  if (s.owner != player_num) return false;

  // Don't report on ships this governor is not authorized for
  if (!authorized(governor, s)) return false;

  // Don't report on ships whose type isn't in the requested report filter
  if (!rep_on[s.type]) return false;

  // Don't report on undocked canisters (launched canisters don't show up)
  if (s.type == ShipType::OTYPE_CANIST && !s.docked) return false;

  // Don't report on undocked greens (launched greens don't show up)
  if (s.type == ShipType::OTYPE_GREEN && !s.docked) return false;

  return true;
}

// ============================================================================
// PlanetReportItem Method Implementations
// ============================================================================

/// Check if a planet should be included in the report.
///
/// Planets are included only if the player owns at least one sector on them.
///
/// \return true if planet should be included in report
bool PlanetReportItem::should_report(
    player_t player_num, [[maybe_unused]] governor_t governor,
    [[maybe_unused]] const ReportArray& rep_on) const {
  // Don't report on planets where we don't own any sectors
  return planet_->info[player_num - 1].numsectsowned != 0;
}

// ============================================================================
// Tactical Report Methods
// ============================================================================

/// Display a ship as a tactical target in another entity's tactical report.
///
/// Shows the ship's distance, hit probability, size, speed, evasion, damage,
/// and location. Applies various filters:
/// - Player filter (ctx.who)
/// - Ship type filter (ctx.shiplist)
/// - Ownership filter (don't show own ships)
/// - Alive status filter
/// - Special ship type filter (canisters, greenhouse gases)
/// - Enemies-only filter (ctx.enemies_only)
///
/// Calculates hit probability based on distance, technology, evasion, speed,
/// and other combat factors.
void ShipReportItem::print_tactical_target(
    GameObj& g, [[maybe_unused]] RstContext& ctx, const Race& race,
    [[maybe_unused]] shipnum_t indx, double dist, double tech, int fdam,
    bool fev, int fspeed, guntype_t caliber) const {
  const player_t player_num = g.player;
  const governor_t governor = g.governor;

  const auto& s = *ship_;

  // Filter ships not matching the "who" filter (player filter or ship type
  // list)
  if (ctx.who && ctx.who != s.owner &&
      (ctx.who != 999 || !listed((int)s.type, ctx.shiplist))) {
    return;
  }

  // Don't show ships we own and are authorized for
  if (s.owner == player_num && authorized(governor, s)) {
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

  // Calculate combat parameters
  int body = size(s);
  auto defense = getdefense(s);
  auto [prob, factor] = hit_odds(dist, tech, fdam, fev, tev, fspeed, tspeed,
                                 body, caliber, defense);

  // Apply laser focus bonus if applicable
  prob = ctx.rd[indx]->apply_laser_focus(prob);

  // Determine diplomatic status indicator
  const auto* war_status = isset(race.atwar, s.owner)    ? "-"
                           : isset(race.allied, s.owner) ? "+"
                                                         : " ";

  // Filter out allied ships if enemies-only mode is enabled
  if (ctx.enemies_only && isset(race.allied, s.owner)) {
    return;
  }

  // Output tactical information for this target ship
  g.out << std::format(
      "{:13} {}{:2},{:1} {:c}{:14.14} {:4.0f}  {:4}   {:4} {}  "
      "{:3}  "
      "{:3}% {:3}%{}",
      n_, war_status, s.owner, s.governor, Shipltrs[s.type], s.name, dist,
      factor, body, tspeed, (tev ? "yes" : "   "), prob, s.damage,
      (s.active ? "" : " INACTIVE"));

  if (landed(s)) {
    g.out << std::format(" ({},{})", s.land_x, s.land_y);
  } else {
    g.out << "     ";
  }
  g.out << "\n";
}

/// Apply laser focus bonus to hit probability.
///
/// When a ship has an active laser and focus enabled, the hit probability
/// is squared (prob * prob / 100), making focused laser fire significantly
/// more accurate.
///
/// \param prob Base hit probability (0-100)
/// \return Modified hit probability with laser focus applied
int ShipReportItem::apply_laser_focus(int prob) const {
  if (laser_on(*ship_) && ship_->focus) {
    return prob * prob / 100;
  }
  return prob;
}

/// Print tactical header summary for a ship.
///
/// Displays the firing ship's combat-relevant stats including tech, guns,
/// armor, size, speed, evasion, and current location.
void ShipReportItem::print_tactical_header_summary(
    GameObj& g, [[maybe_unused]] player_t player_num,
    const TacticalParams& params) const {
  const auto& s = *ship_;
  Place where{s.whatorbits, s.storbits, s.pnumorbits};
  auto orb = std::format("{:30.30}", where.to_string());
  g.out << std::format(
      "{:3} {:c} {:16.16} "
      "{:4.0f}{:3}{:c}/{:3}{:c}{:6}{:5}{:5}{:7.1f}{:3}%  {}  "
      "{:3}{:21.22}",
      n_, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"), s.tech, s.primary,
      kCaliber[s.primtype], s.secondary, kCaliber[s.sectype], s.armor, s.size,
      s.destruct, s.fuel, s.damage, params.fspeed, (params.fev ? "yes" : "   "),
      orb);
  if (landed(s)) {
    g.out << std::format(" ({},{})", s.land_x, s.land_y);
  }
  if (!s.active) {
    g.out << std::format(" INACTIVE({})", s.rad);
  }
  g.out << "\n";
}

/// Get tactical combat parameters for this ship.
///
/// Returns the ship's technology level, speed, and evasion status.
/// Speed and evasion are only set if the ship is actively moving
/// (has a destination or navigation on, not docked, and active).
///
/// \return TacticalParams with ship's combat parameters
TacticalParams ShipReportItem::get_tactical_params(
    [[maybe_unused]] const Race& race) const {
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

/// Display a planet as a tactical target.
///
/// Shows the planet name and distance. Planets don't have the detailed
/// combat stats that ships have, so this is a simplified display.
void PlanetReportItem::print_tactical_target(
    GameObj& g, [[maybe_unused]] RstContext& ctx,
    [[maybe_unused]] const Race& race, [[maybe_unused]] shipnum_t indx,
    double dist, [[maybe_unused]] double tech, [[maybe_unused]] int fdam,
    [[maybe_unused]] bool fev, [[maybe_unused]] int fspeed,
    [[maybe_unused]] guntype_t caliber) const {
  const auto* star = g.entity_manager.peek_star(star_);
  g.out << std::format(" {:13}(planet)          {:8.0f}\n",
                       star ? star->pnames[pnum_] : "Unknown", dist);
}

/// Print tactical header summary for a planet.
///
/// Shows the planet's defensive capabilities including tech level (from race),
/// number of guns, destruct stores, and fuel.
void PlanetReportItem::print_tactical_header_summary(
    GameObj& g, player_t player_num, const TacticalParams& params) const {
  const auto& p = *planet_;
  const auto* star = g.entity_manager.peek_star(star_);

  g.out << std::format("(planet){:15.15}{:4.0f} {:4}M           {:5} {:6}\n",
                       star ? star->pnames[pnum_] : "Unknown", params.tech,
                       p.info[player_num - 1].guns,
                       p.info[player_num - 1].destruct,
                       p.info[player_num - 1].fuel);
}

/// Get tactical combat parameters for a planet.
///
/// Returns the race's technology level. Planets don't have speed or evasion.
///
/// \param race The race that owns sectors on this planet
/// \return TacticalParams with race technology
TacticalParams PlanetReportItem::get_tactical_params(const Race& race) const {
  TacticalParams params{};
  params.tech = race.tech;
  // Planets don't have speed or evasion
  return params;
}

/// Generate tactical combat report for a ship.
///
/// Shows the ship's current status and all nearby targets within gun range.
/// For each target, displays distance, hit probability, size, speed, evasion,
/// and damage status. Only shows targets if the ship has sight capability.
///
/// The report includes:
/// - Ship's header summary with combat stats
/// - List of all targets within gun range
/// - Hit probabilities calculated based on distance, speed, evasion, etc.
void ShipReportItem::report_tactical(GameObj& g, RstContext& ctx,
                                     const TacticalParams& params) const {
  if (!ctx.flags.tactical) return;

  const player_t player_num = g.player;
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) return;

  const auto& s = *ship_;

  g.out << "\n  #         name        tech    guns  armor size dest   "
           "fuel dam spd evad               orbits\n";
  print_tactical_header_summary(g, player_num, params);

  bool sight = shipsight(s);
  if (!sight) return;

  /* tactical display */
  g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
           "spd evade hit  dam  loc\n";

  for (shipnum_t i = 0; i < ctx.rd.size(); i++) {
    // Skip ourselves
    if (ctx.rd[i]->x() == x_ && ctx.rd[i]->y() == y_) continue;

    double range = gun_range(s);
    double dist = sqrt(Distsq(x_, y_, ctx.rd[i]->x(), ctx.rd[i]->y()));
    if (dist >= range) continue;

    // Calculate ship-specific combat parameters
    int fdam = s.damage;
    guntype_t caliber = current_caliber(s);

    // Polymorphic call - works for both ships and planets
    ctx.rd[i]->print_tactical_target(g, ctx, *race, i, dist, params.tech, fdam,
                                     params.fev, params.fspeed, caliber);
  }
}

/// Generate tactical combat report for a planet.
///
/// Shows the planet's defensive capabilities and all nearby targets within
/// gun range. Similar to ship tactical but uses race tech instead of ship tech,
/// and planets don't have damage or ship-specific gun calibers.
///
/// The report includes:
/// - Planet's header summary with defensive stats
/// - List of all targets within gun range
/// - Distance to each target
void PlanetReportItem::report_tactical(GameObj& g, RstContext& ctx,
                                       const TacticalParams& params) const {
  if (!ctx.flags.tactical) return;

  const player_t player_num = g.player;
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) return;

  g.out << "\n  #         name        tech    guns  armor size dest   "
           "fuel dam spd evad               orbits\n";
  print_tactical_header_summary(g, player_num, params);

  /* tactical display */
  g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
           "spd evade hit  dam  loc\n";

  for (shipnum_t i = 0; i < ctx.rd.size(); i++) {
    // Skip ourselves
    if (ctx.rd[i]->x() == x_ && ctx.rd[i]->y() == y_) continue;

    double range = gun_range(*race);
    double dist = sqrt(Distsq(x_, y_, ctx.rd[i]->x(), ctx.rd[i]->y()));
    if (dist >= range) continue;

    // Calculate ship-specific combat parameters from planet perspective
    int fdam = 0;  // Planets don't have damage
    guntype_t caliber = GTYPE_MEDIUM;

    // Polymorphic call - works for both ships and planets
    ctx.rd[i]->print_tactical_target(g, ctx, *race, i, dist, params.tech, fdam,
                                     params.fev, params.fspeed, caliber);
  }
}

// ============================================================================
// Main Report Function
// ============================================================================

/// Generate all requested reports for a single item (ship or planet).
///
/// This is the main entry point for generating reports. It checks if the item
/// should be reported based on filters, then calls all active report methods
/// in sequence based on the report flags.
///
/// \param g Game context
/// \param ctx Report context with flags and state
/// \param item The ship or planet to report on
/// \param rep_on Array indicating which ship types to include
void ship_report(GameObj& g, RstContext& ctx, const ReportItem& item,
                 const ReportArray& rep_on) {
  player_t player_num = g.player;
  governor_t governor = g.governor;

  // Get race from EntityManager
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  // Check if this item should be reported
  if (!item.should_report(player_num, governor, rep_on)) {
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
// Command Entry Point
// ============================================================================

namespace GB::commands {
/// Main entry point for ship and planet reporting commands.
///
/// This command supports multiple report types through different command names:
/// - report: General location and status report
/// - stock: Resource and inventory report
/// - tactical: Combat situation report with targets
/// - stats: Technology and capabilities report
/// - weapons: Weapons systems report
/// - factories: Factory ship build configuration report
/// - ship: Comprehensive report (all of the above)
///
/// Usage patterns:
/// 1. `<cmd>` - Report all ships at current scope
/// 2. `<cmd> <types>` - Report ships of specified types (e.g., "dsc" for destroyers/scouts/cruisers)
/// 3. `<cmd> #<shipno> [#<shipno> ...]` - Report specific ships by number
/// 4. `<cmd> <types> <player>` - Filter by ship types and player number
/// 5. `<cmd> <types> <ship_types>` - Filter by ship types appearing in another ship type list
///
/// \param argv Command arguments (argv[0] is the command name)
/// \param g Game context with player, governor, and scope information
void rst(const command_t& argv, GameObj& g) {
  const player_t player_num = g.player;

  ReportArray report_types;
  report_types.fill(true);

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
  ctx.rd.reserve(n_ships + Sdata.numstars * MAXPLANETS);
  /* (one list entry for each ship, planet in universe) */

  // Parse optional third argument: player number or ship type filter
  if (argv.size() == 3) {
    if (isdigit(argv[2][0]))
      ctx.who = std::stoi(argv[2]);
    else {
      ctx.who = 999; /* treat argv[2].c_str() as a list of ship types */
      ctx.shiplist = argv[2];
    }
  } else
    ctx.who = 0;

  // Parse second argument: ship numbers or ship type filter
  if (argv.size() >= 2) {
    if (*argv[1].c_str() == '#' || isdigit(*argv[1].c_str())) {
      // Report on specific ships by number
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
          star_get_report_ships(g, ctx, player_num, *star_opt);
          ship_report(g, ctx, *ctx.rd.back(), report_types);
        } else {
          ship_report(g, ctx, *ctx.rd.back(), report_types);
        }
        l++;
      }
      return;
    }
    
    // Parse ship type filter string
    report_types.fill(false);

    for (const auto& c : argv[1]) {
      shipnum_t i = NUMSTYPES;
      while (--i && Shipltrs[i] != c);
      if (Shipltrs[i] != c) {
        g.out << std::format("'{}' -- no such ship letter\n", c);
      } else
        report_types[i] = true;
    }
  }

  // Generate reports based on current scope level
  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      // Universe level: report all ships everywhere
      if (!ctx.flags.tactical || argv.size() >= 2) {
        // Collect ships in deep space
        shipnum_t shn = Sdata.ships;
        while (shn && get_report_ship(g, ctx, shn)) {
          shn = ctx.rd.back()->next_ship();
        }

        // Collect ships in all star systems
        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_get_report_ships(g, ctx, player_num, i);
        
        // Generate reports for all collected items
        for (const auto& item : ctx.rd)
          ship_report(g, ctx, *item, report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
      
    case ScopeLevel::LEVEL_PLAN:
      // Planet level: report ships at this planet
      plan_get_report_ships(g, ctx, player_num, g.snum, g.pnum);
      for (const auto& item : ctx.rd) ship_report(g, ctx, *item, report_types);
      break;
      
    case ScopeLevel::LEVEL_STAR:
      // Star system level: report all ships in this star system
      star_get_report_ships(g, ctx, player_num, g.snum);
      for (const auto& item : ctx.rd) ship_report(g, ctx, *item, report_types);
      break;
      
    case ScopeLevel::LEVEL_SHIP:
      // Ship level: report this ship and any ships docked/landed on it
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

      // Get ships docked/landed on this ship
      shipnum_t shn = ctx.rd[0]->child_ships();
      shipnum_t first_child = ctx.rd.size();

      while (shn && get_report_ship(g, ctx, shn)) {
        shn = ctx.rd.back()->next_ship();
      }

      for (shipnum_t i = first_child; i < ctx.rd.size(); i++)
        ship_report(g, ctx, *ctx.rd[i], report_types);
      break;
  }
}
}  // namespace GB::commands
}  // namespace GB::commands