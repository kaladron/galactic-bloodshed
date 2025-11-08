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

module commands;

namespace {
constexpr char kCaliber[] = {' ', 'L', 'M', 'H'};

using ReportArray = std::array<bool, NUMSTYPES>;

// Forward declarations
struct RstContext;

// Tactical parameters for combat calculations
struct TacticalParams {
  double tech = 0.0;
  bool fev = false;
  int fspeed = 0;
};

// Abstract base class for report items (ships and planets)
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
      [[maybe_unused]] shipnum_t indx,
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
  void report_tactical(GameObj& g, RstContext& ctx, shipnum_t indx,
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

  shipnum_t next_ship() const override;

  shipnum_t child_ships() const override;

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

  void report_tactical(GameObj& g, RstContext& ctx, shipnum_t indx,
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

// Report mode flags - all default to false
struct ReportFlags {
  bool status = false;
  bool ship = false;
  bool stock = false;
  bool report = false;
  bool weapons = false;
  bool factories = false;
  bool tactical = false;
};

// Context structure holding all rst command state
struct RstContext {
  std::vector<std::unique_ptr<ReportItem>> rd;
  std::string shiplist;
  ReportFlags flags;
  bool first;
  bool enemies_only;
  int who;
  shipnum_t num_ships = 0;  // Local counter instead of global Num_ships
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

bool listed(int type, const std::string& string) {
  return std::ranges::any_of(string,
                             [type](char c) { return Shipltrs[type] == c; });
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
bool get_report_ship(GameObj& g, RstContext& ctx, shipnum_t shipno) {
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (ship) {
    ctx.rd.push_back(std::make_unique<ShipReportItem>(shipno, ship));
    ctx.num_ships++;
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
  ctx.num_ships++;

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

// ShipReportItem virtual method implementations
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

bool PlanetReportItem::should_report(
    player_t player_num, [[maybe_unused]] governor_t governor,
    [[maybe_unused]] const ReportArray& rep_on) const {
  // Don't report on planets where we don't own any sectors
  return planet_->info[player_num - 1].numsectsowned != 0;
}

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

shipnum_t ShipReportItem::next_ship() const { return ship_->nextship; }

shipnum_t ShipReportItem::child_ships() const { return ship_->ships; }

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
                                     shipnum_t indx,
                                     const TacticalParams& params) const {
  if (!ctx.flags.tactical) return;

  const player_t player_num = g.player;
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) return;

  const auto& s = *ship_;

  g.out << "\n  #         name        tech    guns  armor size dest   "
           "fuel dam spd evad               orbits\n";
  ctx.rd[indx]->print_tactical_header_summary(g, player_num, params);

  bool sight = shipsight(s);
  if (!sight) return;

  /* tactical display */
  g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
           "spd evade hit  dam  loc\n";

  for (int i = 0; i < ctx.num_ships; i++) {
    if (i == indx) continue;

    double range = gun_range(s);
    double dist = sqrt(Distsq(ctx.rd[indx]->x(), ctx.rd[indx]->y(),
                              ctx.rd[i]->x(), ctx.rd[i]->y()));
    if (dist >= range) continue;

    // Calculate ship-specific combat parameters
    int fdam = s.damage;
    guntype_t caliber = current_caliber(s);

    // Polymorphic call - works for both ships and planets
    ctx.rd[i]->print_tactical_target(g, ctx, *race, indx, dist, params.tech,
                                     fdam, params.fev, params.fspeed, caliber);
  }
}

void PlanetReportItem::report_tactical(GameObj& g, RstContext& ctx,
                                       shipnum_t indx,
                                       const TacticalParams& params) const {
  if (!ctx.flags.tactical) return;

  const player_t player_num = g.player;
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) return;

  g.out << "\n  #         name        tech    guns  armor size dest   "
           "fuel dam spd evad               orbits\n";
  ctx.rd[indx]->print_tactical_header_summary(g, player_num, params);

  /* tactical display */
  g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
           "spd evade hit  dam  loc\n";

  for (int i = 0; i < ctx.num_ships; i++) {
    if (i == indx) continue;

    double range = gun_range(*race);
    double dist = sqrt(Distsq(ctx.rd[indx]->x(), ctx.rd[indx]->y(),
                              ctx.rd[i]->x(), ctx.rd[i]->y()));
    if (dist >= range) continue;

    // Calculate ship-specific combat parameters from planet perspective
    int fdam = 0;  // Planets don't have damage
    guntype_t caliber = GTYPE_MEDIUM;

    // Polymorphic call - works for both ships and planets
    ctx.rd[i]->print_tactical_target(g, ctx, *race, indx, dist, params.tech,
                                     fdam, params.fev, params.fspeed, caliber);
  }
}

void ship_report(GameObj& g, RstContext& ctx, shipnum_t indx,
                 const ReportArray& rep_on) {
  player_t player_num = g.player;
  governor_t governor = g.governor;

  // Bounds check - ensure indx is within the vector
  if (indx >= ctx.rd.size()) {
    return;
  }

  // Get race from EntityManager
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  // Check if this item should be reported
  if (!ctx.rd[indx]->should_report(player_num, governor, rep_on)) {
    return;
  }

  // Calculate tactical parameters for report_tactical (polymorphic)
  TacticalParams params = ctx.rd[indx]->get_tactical_params(*race);

  // Polymorphic dispatch to appropriate report methods
  ctx.rd[indx]->report_stock(g, ctx);
  ctx.rd[indx]->report_status(g, ctx);
  ctx.rd[indx]->report_weapons(g, ctx);
  ctx.rd[indx]->report_factories(g, ctx);
  ctx.rd[indx]->report_general(g, ctx);
  ctx.rd[indx]->report_tactical(g, ctx, indx, params);
}
}  // namespace

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
          ship_report(g, ctx, ctx.num_ships - 1, report_types);
        } else {
          ship_report(g, ctx, ctx.num_ships - 1, report_types);
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
        for (shipnum_t i = 0; i < ctx.rd.size(); i++)
          ship_report(g, ctx, i, report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_get_report_ships(g, ctx, player_num, g.snum, g.pnum);
      for (shipnum_t i = 0; i < ctx.rd.size(); i++)
        ship_report(g, ctx, i, report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_get_report_ships(g, ctx, player_num, g.snum);
      for (shipnum_t i = 0; i < ctx.rd.size(); i++)
        ship_report(g, ctx, i, report_types);
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
      ship_report(g, ctx, 0, report_types); /* first ship report */

      // Get ships docked in this ship
      shipnum_t shn = ctx.rd[0]->child_ships();
      shipnum_t first_child = ctx.num_ships;

      while (shn && get_report_ship(g, ctx, shn)) {
        shn = ctx.rd.back()->next_ship();
      }

      for (shipnum_t i = first_child; i < ctx.rd.size(); i++)
        ship_report(g, ctx, i, report_types);
      break;
  }
}
}  // namespace GB::commands