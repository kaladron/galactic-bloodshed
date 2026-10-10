// SPDX-License-Identifier: Apache-2.0

/// \file orbit.cc
/// \brief Display orbits of planets (graphic representation).

module;

import std;
import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import scnlib;

module commands;

namespace {

constexpr double ORBIT_SCALE = 100.0;
constexpr SystemCoordinates ORBIT_CENTER{ORBIT_SCALE, ORBIT_SCALE};

struct OrbitViewport {
  UniverseCoordinates universe_center{0.0, 0.0};
  SystemCoordinates system_center{0.0, 0.0};
  double zoom{1.0};
};

struct OrbitOptions {
  int suppress_id{-1};
  bool show_planet_names{true};
  bool show_ships{true};
  bool show_star_names{true};
  std::optional<std::string_view> path_arg{std::nullopt};
};

SystemCoordinates project_orbit_coords(SystemCoordinates offset,
                                       double max_extent, double zoom) {
  return ORBIT_CENTER + offset * (ORBIT_SCALE / (max_extent * zoom));
}

bool parse_flag_group(std::string_view flags, OrbitOptions& opts, GameObj& g) {
  for (const char c : flags) {
    switch (c) {
      case 's':
        opts.show_ships = false;
        break;
      case 'S':
        opts.show_star_names = false;
        break;
      case 'p':
        opts.show_planet_names = false;
        break;
      default: {
        auto scan_res = scn::scan<int>(flags, "{}");
        if (!scan_res) {
          g.out << std::format("Bad number {}.\n", flags);
          return false;
        }
        opts.suppress_id = scan_res->value();
        return true;
      }
    }
  }
  return true;
}

std::optional<OrbitOptions> parse_orbit_options(const command_t& argv,
                                                GameObj& g) {
  OrbitOptions opts{};
  for (const std::string_view arg : argv | std::views::drop(1)) {
    if (!arg.starts_with('-')) {
      opts.path_arg = arg;
      continue;
    }
    if (!parse_flag_group(arg.substr(1), opts, g)) {
      return std::nullopt;
    }
  }
  return opts;
}

OrbitViewport compute_viewport(const GameObj& g, bool has_path_arg) {
  if (has_path_arg) {
    return OrbitViewport{.zoom = 1.1};
  }
  return OrbitViewport{
      .universe_center = g.universe_center(),
      .system_center = g.system_center(),
      .zoom = (g.level() == ScopeLevel::LEVEL_UNIV) ? g.zoom[1] : g.zoom[0],
  };
}

GB::presentation::OrbitStarEntry
build_star_entry(const GameObj& g, ScopeLevel level, const Star& star,
                 const OrbitViewport& vp, const OrbitOptions& opts) {
  const SystemCoordinates coords =
      (level == ScopeLevel::LEVEL_UNIV)
          ? project_orbit_coords(star.coordinates() - vp.universe_center,
                                 UNIVSIZE, vp.zoom)
          : project_orbit_coords(-vp.system_center, SYSTEMSIZE, vp.zoom);

  return GB::presentation::OrbitStarEntry{
      .coords = coords,
      .symbol_standout = star.is_explored_by(g.player()),
      .label_standout = star.is_inhabited_by(g.player()),
      .label = opts.show_star_names ? star.get_name() : std::string{},
  };
}

GB::presentation::OrbitPlanetEntry
build_planet_entry(const GameObj& g, ScopeLevel level, const Planet& p,
                   std::string_view name, const Race& r,
                   const OrbitViewport& vp, const OrbitOptions& opts) {
  const SystemCoordinates coords =
      (level == ScopeLevel::LEVEL_STAR)
          ? project_orbit_coords(p.system_coordinates() - vp.system_center,
                                 SYSTEMSIZE, vp.zoom)
          : project_orbit_coords(-vp.system_center, PLORBITSIZE, vp.zoom);

  const bool explored = p.info(g.player()).explored;
  std::optional<Percentage> compat = std::nullopt;
  if (opts.show_planet_names && r.governor(g.governor()).toggle.compat &&
      explored) {
    compat = Percentage{p.compatibility(r)};
  }

  return GB::presentation::OrbitPlanetEntry{
      .coords = coords,
      .symbol = explored ? p.type_symbol() : '?',
      .symbol_standout = explored,
      .label_standout = p.info(g.player()).numsectsowned > 0,
      .label = opts.show_planet_names ? std::string(name) : std::string{},
      .compatibility = compat,
  };
}

std::optional<GB::presentation::OrbitShipEntry>
build_ship_entry(const GameObj& g, EntityManager& em, const Place& where,
                 const Ship& ship, const Race& r, const OrbitViewport& vp,
                 const Planet* pl = nullptr) {
  bearing_t mirror_heading = 0;
  switch (ship.type()) {
    case ShipType::STYPE_MIRROR: {
      const auto* mirror = ship.as<SpaceMirrorShip>();
      mirror_heading = em.resolve_mirror_aim_direction(*mirror);
      break;
    }
    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      return std::nullopt;
    default:
      if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV &&
          ship.owner() != g.player() && !g.god()) {
        return std::nullopt;
      }
      break;
  }

  SystemCoordinates coords{};
  if (where.level == ScopeLevel::LEVEL_PLAN) {
    const auto& where_star = *em.peek_star(where.snum);
    const UniverseCoordinates pl_coords = pl->absolute_coordinates(where_star);
    coords = project_orbit_coords((ship.coordinates() - pl_coords) -
                                      vp.system_center,
                                  PLORBITSIZE, vp.zoom);
  } else if (where.level == ScopeLevel::LEVEL_STAR) {
    const auto& where_star = *em.peek_star(where.snum);
    coords = project_orbit_coords(
        (ship.coordinates() - where_star.coordinates()) - vp.system_center,
        SYSTEMSIZE, vp.zoom);
  } else {
    coords = project_orbit_coords(ship.coordinates() - vp.universe_center,
                                  UNIVSIZE, vp.zoom);
  }

  if (coords.x < 0.0 || coords.y < 0.0) {
    return std::nullopt;
  }

  return GB::presentation::OrbitShipEntry{
      .coords = coords,
      .mirror_heading = mirror_heading,
      .symbol = ship.type_letter(),
      .standout = (ship.owner() == r.governor(g.governor()).toggle.highlight),
      .shipno = ship.number(),
  };
}

bool has_system_sight(EntityManager& em, starnum_t snum, player_t player,
                      bool god) {
  if (god) return true;
  for (const Ship& s : ShipList::readonly(em, snum)) {
    if (s.owner() == player && s.has_sight()) return true;
  }
  return false;
}

bool has_planet_sight(EntityManager& em, starnum_t snum, planetnum_t pnum,
                      player_t player) {
  for (const Ship& s : ShipList::readonly(em, snum, pnum)) {
    if (s.owner() == player && s.has_sight()) return true;
  }
  return false;
}

std::string
format_legacy_orbit_wire(const GB::presentation::OrbitViewModel& vm) {
  std::string out = "#";
  for (const auto& star : vm.stars) {
    std::format_to(std::back_inserter(out), "{} {} {} 0 * {} {};",
                   star.symbol_standout ? 1 : 0,
                   static_cast<int>(star.coords.x),
                   static_cast<int>(star.coords.y), star.label_standout ? 1 : 0,
                   star.label);
  }
  for (const auto& planet : vm.planets) {
    std::format_to(std::back_inserter(out), "{} {} {} 0 {} {} {}",
                   planet.symbol_standout ? 1 : 0,
                   static_cast<int>(planet.coords.x),
                   static_cast<int>(planet.coords.y), planet.symbol,
                   planet.label_standout ? 1 : 0, planet.label);
    if (planet.compatibility) {
      std::format_to(std::back_inserter(out), "({})", *planet.compatibility);
    }
    out.push_back(';');
  }
  for (const auto& ship : vm.ships) {
    const int stand = ship.standout ? 1 : 0;
    std::format_to(std::back_inserter(out), "{} {} {} {} {} {} {};", stand,
                   static_cast<int>(ship.coords.x),
                   static_cast<int>(ship.coords.y), ship.mirror_heading,
                   ship.symbol, stand, ship.shipno);
  }
  out.push_back('\n');
  return out;
}

}  // namespace

namespace GB::commands {

bool orbit(const command_t& argv, GameObj& g) {
  const auto opts = parse_orbit_options(argv, g);
  if (!opts) return false;

  const auto resolved = Place::resolve(g.entity_manager, g.scope_context(),
                                       opts->path_arg.value_or(":"));
  if (!resolved) {
    g.out << format_place_error(resolved.error());
    g.out << "orbit: error in args.\n";
    return false;
  }
  const Place& where = *resolved;
  const OrbitViewport vp = compute_viewport(g, opts->path_arg.has_value());
  const Race& race = *g.race;

  GB::presentation::OrbitViewModel vm{
      .inverse = race.governor(g.governor()).toggle.inverse,
  };

  switch (where.level) {
    case ScopeLevel::LEVEL_UNIV: {
      for (const Star& star_ref : StarList::readonly(g.entity_manager)) {
        if (opts->suppress_id != star_ref.star_id()) {
          vm.stars.push_back(
              build_star_entry(g, ScopeLevel::LEVEL_UNIV, star_ref, vp, *opts));
        }
      }
      if (opts->show_ships) {
        for (const Ship& s :
             ShipList::readonly(g.entity_manager, ScopeLevel::LEVEL_UNIV)) {
          if (opts->suppress_id != s.number()) {
            if (auto entry =
                    build_ship_entry(g, g.entity_manager, where, s, race, vp)) {
              vm.ships.push_back(*entry);
            }
          }
        }
      }
      break;
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto& star = *g.entity_manager.peek_star(where.snum);
      vm.stars.push_back(
          build_star_entry(g, ScopeLevel::LEVEL_STAR, star, vp, *opts));

      for (const auto& p : PlanetList::readonly(g.entity_manager, where.snum,
                                                star.numplanets())) {
        if (opts->suppress_id != p.planet_order()) {
          vm.planets.push_back(build_planet_entry(
              g, ScopeLevel::LEVEL_STAR, p,
              star.get_planet_name(p.planet_order()), race, vp, *opts));
        }
      }
      const bool iq =
          has_system_sight(g.entity_manager, where.snum, g.player(), g.god());
      if (opts->show_ships) {
        for (const Ship& s : ShipList::readonly(g.entity_manager, where.snum)) {
          const bool is_enemy_mine =
              s.owner() != g.player() && s.type() == ShipType::STYPE_MINE;
          if (opts->suppress_id != s.number() && !is_enemy_mine &&
              (s.owner() == g.player() || iq)) {
            if (auto entry =
                    build_ship_entry(g, g.entity_manager, where, s, race, vp)) {
              vm.ships.push_back(*entry);
            }
          }
        }
      }
      break;
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto& plan_star = *g.entity_manager.peek_star(where.snum);
      const auto& p = *g.entity_manager.peek_planet(where.snum, where.pnum);
      vm.planets.push_back(build_planet_entry(
          g, ScopeLevel::LEVEL_PLAN, p, plan_star.get_planet_name(where.pnum),
          race, vp, *opts));

      const bool iq = has_planet_sight(g.entity_manager, where.snum, where.pnum,
                                       g.player());
      if (opts->show_ships) {
        for (const Ship& s :
             ShipList::readonly(g.entity_manager, where.snum, where.pnum)) {
          if (opts->suppress_id != s.number() && !s.is_landed() &&
              (s.owner() == g.player() || iq)) {
            if (auto entry = build_ship_entry(g, g.entity_manager, where, s,
                                              race, vp, &p)) {
              vm.ships.push_back(*entry);
            }
          }
        }
      }
      break;
    }
    default:
      g.out << "Bad scope.\n";
      return false;
  }

  g.out << format_legacy_orbit_wire(vm);
  return true;
}

const CommandDescriptor orbit_cmd{
    .name = "orbit",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "orbit [-p] [-S] [-s] [-<num>] [<path>]",
    .description = "Graphic representation of objects in current scope or path",
    .handler = &orbit,
};

}  // namespace GB::commands
