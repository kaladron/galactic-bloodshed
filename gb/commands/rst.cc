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
// CONSTANTS
// ============================================================================

// (No global constants needed - caliber mapping is in caliber_char() function)

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

using ReportArray = std::array<bool, NUMSTYPES>;

// Forward declarations
class ReportItem;
struct RstContext;

struct TacticalParams {
  double tech = 0.0;
  bool fev = false;
  int fspeed = 0;
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
  int who;
};

// ============================================================================
// BASE CLASS: ReportItem
// ============================================================================

class ReportItem {
 protected:
  double x_;
  double y_;

 public:
  ReportItem(double x, double y) : x_(x), y_(y) {}
  virtual ~ReportItem() = default;

  double x() const { return x_; }
  double y() const { return y_; }

  // Report generation methods
  virtual void report_stock([[maybe_unused]] GameObj& g,
                            [[maybe_unused]] RstContext& ctx) const {}
  virtual void report_status([[maybe_unused]] GameObj& g,
                             [[maybe_unused]] RstContext& ctx) const {}
  virtual void report_weapons([[maybe_unused]] GameObj& g,
                              [[maybe_unused]] RstContext& ctx) const {}
  virtual void report_factories([[maybe_unused]] GameObj& g,
                                [[maybe_unused]] RstContext& ctx) const {}
  virtual void report_general([[maybe_unused]] GameObj& g,
                              [[maybe_unused]] RstContext& ctx) const {}
  virtual void report_tactical(
      [[maybe_unused]] GameObj& g, [[maybe_unused]] RstContext& ctx,
      [[maybe_unused]] const TacticalParams& params) const {}

  // Tactical target display - default no-op for items that don't appear in
  // tactical
  virtual void print_tactical_target(
      [[maybe_unused]] GameObj& g, [[maybe_unused]] RstContext& ctx,
      [[maybe_unused]] const Race& race, [[maybe_unused]] shipnum_t indx,
      [[maybe_unused]] double dist, [[maybe_unused]] double tech,
      [[maybe_unused]] int fdam, [[maybe_unused]] bool fev,
      [[maybe_unused]] int fspeed, [[maybe_unused]] guntype_t caliber) const {}

  // Apply laser focus bonus to hit probability (ships only)
  virtual int apply_laser_focus(int prob) const { return prob; }

  // Print tactical report header summary (different for ships vs planets)
  virtual void print_tactical_header_summary(
      [[maybe_unused]] GameObj& g, [[maybe_unused]] player_t player_num,
      [[maybe_unused]] const TacticalParams& params) const {}

  // Get tactical parameters for this item
  virtual TacticalParams get_tactical_params(
      [[maybe_unused]] const Race& race) const {
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
                             const ReportArray& rep_on) const = 0;
};

// ============================================================================
// DERIVED CLASSES
// ============================================================================

// Ship report item - holds non-owning pointer from peek_ship
class ShipReportItem : public ReportItem {
  shipnum_t n_;
  const Ship* ship_;

 public:
  ShipReportItem(shipnum_t n, const Ship* ship)
      : ReportItem(ship->xpos, ship->ypos), n_(n), ship_(ship) {}

  shipnum_t shipno() const { return n_; }
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

  std::optional<starnum_t> get_star_orbit() const override;

  shipnum_t next_ship() const override { return ship_->nextship; }

  shipnum_t child_ships() const override { return ship_->ships; }

  bool should_report(player_t player_num, governor_t governor,
                     const ReportArray& rep_on) const override;
};

// Planet report item - holds non-owning pointer from peek_planet
class PlanetReportItem : public ReportItem {
  starnum_t star_;
  planetnum_t pnum_;
  const Planet* planet_;

 public:
  PlanetReportItem(starnum_t star, planetnum_t pnum, const Planet* planet,
                   double x, double y)
      : ReportItem(x, y), star_(star), pnum_(pnum), planet_(planet) {}

  starnum_t star() const { return star_; }
  planetnum_t pnum() const { return pnum_; }
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

bool listed(int type, const std::string& string) {
  return std::ranges::any_of(string,
                             [type](char c) { return Shipltrs[type] == c; });
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
bool get_report_ship(GameObj& g, RstContext& ctx, shipnum_t shipno) {
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (ship) {
    ctx.rd.push_back(std::make_unique<ShipReportItem>(shipno, ship));
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
  ctx.rd.push_back(
      std::make_unique<PlanetReportItem>(snum, pnum, planet, x, y));

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
      {std::format("{}", n_),
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
      s.hyper_drive.has ? "yes " : "    ", s.primary, caliber_char(s.primtype),
      s.secondary, caliber_char(s.sectype), armor(s), s.tech, max_speed(s),
      shipcost(s), mass(s), size(s));
  if (s.type == ShipType::STYPE_POD) {
    if (std::holds_alternative<PodData>(s.special)) {
      auto pod = std::get<PodData>(s.special);
      g.out << std::format(" ({})", pod.temperature);
    }
  }
  g.out << "\n";
}

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
      caliber_char(s.primtype), s.secondary, caliber_char(s.sectype), s.damage,
      s.type == ShipType::OTYPE_FACTORY ? Shipltrs[s.build_type] : ' ',
      ((s.type == ShipType::OTYPE_TERRA) || (s.type == ShipType::OTYPE_PLOW))
          ? "Standard"
          : s.shipclass);
}

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

  std::string tmpbuf1 =
      s.primtype ? std::format("{:2}{}", s.primary, caliber_char(s.primtype))
                 : "---";

  std::string tmpbuf2 =
      s.sectype ? std::format("{:2}{}", s.secondary, caliber_char(s.sectype))
                : "---";

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
// PLANET REPORT METHOD IMPLEMENTATIONS
// ============================================================================

bool PlanetReportItem::should_report(
    player_t player_num, [[maybe_unused]] governor_t governor,
    [[maybe_unused]] const ReportArray& rep_on) const {
  // Don't report on planets where we don't own any sectors
  return planet_->info[player_num - 1].numsectsowned != 0;
}

// ============================================================================
// TACTICAL REPORT IMPLEMENTATIONS
// ============================================================================

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

int ShipReportItem::apply_laser_focus(int prob) const {
  if (laser_on(*ship_) && ship_->focus) {
    return prob * prob / 100;
  }
  return prob;
}

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
      caliber_char(s.primtype), s.secondary, caliber_char(s.sectype), s.armor,
      s.size, s.destruct, s.fuel, s.damage, params.fspeed,
      (params.fev ? "yes" : "   "), orb);
  if (landed(s)) {
    g.out << std::format(" ({},{})", s.land_x, s.land_y);
  }
  if (!s.active) {
    g.out << std::format(" INACTIVE({})", s.rad);
  }
  g.out << "\n";
}

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

std::optional<starnum_t> ShipReportItem::get_star_orbit() const {
  if (ship_->whatorbits != ScopeLevel::LEVEL_UNIV) {
    return ship_->storbits;
  }
  return std::nullopt;
}

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

TacticalParams PlanetReportItem::get_tactical_params(const Race& race) const {
  TacticalParams params{};
  params.tech = race.tech;
  // Planets don't have speed or evasion
  return params;
}

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
// MAIN REPORT DRIVER
// ============================================================================

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
// COMMAND ENTRY POINT
// ============================================================================

namespace GB::commands {
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

  if (argv.size() == 3) {
    if (isdigit(argv[2][0]))
      ctx.who = std::stoi(argv[2]);
    else {
      ctx.who = 999; /* treat argv[2].c_str() as a list of ship types */
      ctx.shiplist = argv[2];
    }
  } else
    ctx.who = 0;

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
          star_get_report_ships(g, ctx, player_num, *star_opt);
          ship_report(g, ctx, *ctx.rd.back(), report_types);
        } else {
          ship_report(g, ctx, *ctx.rd.back(), report_types);
        }
        l++;
      }
      return;
    }
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

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (!ctx.flags.tactical || argv.size() >= 2) {
        shipnum_t shn = Sdata.ships;
        while (shn && get_report_ship(g, ctx, shn)) {
          shn = ctx.rd.back()->next_ship();
        }

        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_get_report_ships(g, ctx, player_num, i);
        for (const auto& item : ctx.rd)
          ship_report(g, ctx, *item, report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_get_report_ships(g, ctx, player_num, g.snum, g.pnum);
      for (const auto& item : ctx.rd) ship_report(g, ctx, *item, report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_get_report_ships(g, ctx, player_num, g.snum);
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

      // Get ships docked in this ship
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