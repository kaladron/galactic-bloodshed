// SPDX-License-Identifier: Apache-2.0

/// \file proj_fuel.cc
/// \brief Fuel calculation projection command.

module;

import gb.entities;
import gb.services;
import std;
#undef stdout

module commands;

namespace {

struct FuelTripTarget {
  Place dest;
  UniverseCoordinates dest_coords{};
  double dist = 0.0;
  double gravity_factor = 0.0;
  std::string launch_planet_name;
};

struct FuelSimulationResult {
  bool can_complete = false;
  segments_t segments = 0;
  double fuel_used = 0.0;
};

/**
 * @brief Parse and validate the ship argument and movement capability for fuel
 * projection.
 */
const Ship* validate_fuel_ship(const command_t& argv, GameObj& g) {
  if (argv.size() < 2 || argv.size() > 3) {
    g.out << "Invalid number of options.\n\"fuel #<shipnumber> "
             "[destination]\"...\n";
    return nullptr;
  }
  if (argv[1][0] != '#') {
    g.out << "Invalid first option.\n\"fuel #<shipnumber> [destination]\"...\n";
    return nullptr;
  }
  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || *shipno > g.entity_manager.num_ships() || *shipno < 1) {
    g.out << std::format("rst: no such ship {}\n", argv[1]);
    return nullptr;
  }
  const Ship* ship = nullptr;
  try {
    ship = g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return nullptr;
  }
  if (ship->owner() != g.player()) {
    g.out << "You do not own this ship.\n";
    return nullptr;
  }
  if (ship->is_landed() && argv.size() == 2) {
    g.out << "You must specify a destination for landed or docked ships...\n";
    return nullptr;
  }
  if (!ship->speed()) {
    g.out << "That ship is not moving!\n";
    return nullptr;
  }
  if (!ship->max_speed_capacity()) {
    g.out << "That ship does not have a speed rating...\n";
    return nullptr;
  }
  return ship;
}

/**
 * @brief Resolve destination coordinates, exploration visibility, and launch
 * gravity for a fuel projection trip.
 */
std::optional<FuelTripTarget>
resolve_fuel_trip_target(const command_t& argv, GameObj& g, const Ship& ship) {
  double gravity_factor = 0.0;
  std::string plan_buf;
  if (ship.is_landed() && ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    const auto* p =
        g.entity_manager.peek_planet(ship.storbits(), ship.pnumorbits());
    const auto* star_ptr = g.entity_manager.peek_star(ship.storbits());
    if (!p || !star_ptr) {
      g.out << "Planet or star data not found.\n";
      return std::nullopt;
    }
    gravity_factor = p->gravity();
    plan_buf = std::format("/{}/{}", star_ptr->get_name(),
                           star_ptr->get_planet_name(ship.pnumorbits()));
  }

  Place tmpdest = (argv.size() == 2)
                      ? Place{ship.whatdest(), ship.deststar(), ship.destpnum(),
                              ship.destshipno().value_or(0)}
                      : Place{g, argv[2], true};
  if (tmpdest.err) {
    g.out << "fuel:  bad scope.\n";
    return std::nullopt;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_UNIV) {
    g.out << (argv.size() == 2
                  ? "That ship currently has no destination orders...\n"
                  : "Invalid ship destination.\n");
    return std::nullopt;
  }

  UniverseCoordinates dest_coords{};
  if (tmpdest.level == ScopeLevel::LEVEL_SHIP) {
    const auto* tmpship = g.entity_manager.peek_ship(tmpdest.shipno);
    if (!tmpship) {
      g.out << "Destination ship not found.\n";
      return std::nullopt;
    }
    Ship mutable_target(tmpship->get_struct());
    if (!followable(g.entity_manager, ship, mutable_target)) {
      g.out << "The ship's destination is out of range.\n";
      return std::nullopt;
    }
    if (tmpship->owner() != g.player()) {
      g.out << "Nice try.\n";
      return std::nullopt;
    }
    dest_coords = tmpship->coordinates();
  } else if (tmpdest.level == ScopeLevel::LEVEL_PLAN) {
    const auto* dest_star = g.entity_manager.peek_star(tmpdest.snum);
    if (ship.storbits() != tmpdest.snum &&
        (!dest_star || !dest_star->is_explored_by(ship.owner()))) {
      g.out << "You haven't explored the destination system.\n";
      return std::nullopt;
    }
    const auto* p = g.entity_manager.peek_planet(tmpdest.snum, tmpdest.pnum);
    if (!p || !dest_star) {
      g.out << "Destination planet or star not found.\n";
      return std::nullopt;
    }
    dest_coords = p->absolute_coordinates(*dest_star);
  } else if (tmpdest.level == ScopeLevel::LEVEL_STAR) {
    const auto* dest_star = g.entity_manager.peek_star(tmpdest.snum);
    if (!dest_star) {
      g.out << "Destination star not found.\n";
      return std::nullopt;
    }
    dest_coords = dest_star->coordinates();
  }

  const double dist = ship.coordinates().distance_to(dest_coords);
  if (dist <= DIST_TO_LAND) {
    g.out << "That ship is within 10.0 units of the destination.\n";
    return std::nullopt;
  }

  return FuelTripTarget{
      .dest = std::move(tmpdest),
      .dest_coords = dest_coords,
      .dist = dist,
      .gravity_factor = gravity_factor,
      .launch_planet_name = std::move(plan_buf),
  };
}

/**
 * @brief Iteratively simulate the trip starting from maximum fuel capacity to
 * determine the minimum fuel required.
 */
FuelSimulationResult simulate_optimal_fuel(const Ship& ship,
                                           const FuelTripTarget& target,
                                           EntityManager& em) {
  double level = static_cast<double>(ship.max_fuel());
  double fuel_usage = level;
  bool opt_settings = false;
  segments_t number_segments = 0;

  while (true) {
    SimulatedShip tmpship(ship);
    const auto [can_complete, segs] =
        do_trip(target.dest, tmpship, level, target.gravity_factor,
                target.dest_coords, em);
    if (!can_complete) {
      break;
    }
    number_segments = segs;
    fuel_usage = level;
    opt_settings = true;
    if (tmpship.fuel() < 0.05) {
      break;
    }
    level -= tmpship.fuel();
  }

  return FuelSimulationResult{
      .can_complete = opt_settings,
      .segments = number_segments,
      .fuel_used = fuel_usage,
  };
}

/**
 * @brief Format and print current-cargo and optimum-level fuel estimates.
 */
void render_fuel_projections(GameObj& g, const Ship& ship,
                             const FuelTripTarget& target,
                             const FuelSimulationResult& current_res,
                             const FuelSimulationResult& opt_res) {
  SimulatedShip tmpship(ship);
  g.out << std::format(
      "\n  ----- ===== FUEL ESTIMATES ===== ----\n\nAt Current Fuel "
      "Cargo ({:.2f}f):\n",
      tmpship.fuel());
  domass(tmpship, g.entity_manager);
  if (!current_res.can_complete) {
    g.out << "The ship will not be able to complete the trip.\n";
  } else {
    fuel_output(g, target.dist, current_res.fuel_used, target.gravity_factor,
                tmpship.mass(), current_res.segments,
                target.launch_planet_name);
  }

  g.out << std::format("At Optimum Fuel Level ({:.2f}f):\n", opt_res.fuel_used);
  if (!opt_res.can_complete) {
    g.out << "The ship will not be able to complete the trip.\n";
  } else {
    tmpship.set_simulated_fuel(opt_res.fuel_used);
    domass(tmpship, g.entity_manager);
    fuel_output(g, target.dist, opt_res.fuel_used, target.gravity_factor,
                tmpship.mass(), opt_res.segments, target.launch_planet_name);
  }
}

}  // namespace

namespace GB::commands {

bool proj_fuel(const command_t& argv, GameObj& g) {
  const Ship* ship = validate_fuel_ship(argv, g);
  if (!ship) {
    return false;
  }

  auto target_opt = resolve_fuel_trip_target(argv, g, *ship);
  if (!target_opt) {
    return false;
  }
  const FuelTripTarget& target = *target_opt;

  SimulatedShip fuelcheckship(*ship);
  const double initial_fuel = fuelcheckship.fuel();
  const auto [current_settings, current_segs] =
      do_trip(target.dest, fuelcheckship, initial_fuel, target.gravity_factor,
              target.dest_coords, g.entity_manager);
  const FuelSimulationResult current_res{
      .can_complete = current_settings,
      .segments = current_segs,
      .fuel_used =
          current_settings ? (initial_fuel - fuelcheckship.fuel()) : 0.0,
  };

  const FuelSimulationResult opt_res =
      simulate_optimal_fuel(*ship, target, g.entity_manager);
  render_fuel_projections(g, *ship, target, current_res, opt_res);
  return true;
}

const CommandDescriptor fuel_cmd{
    .name = "fuel",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "fuel <#ship> [<destination>]",
    .description = "Show fuel requirements and travel time for a trip",
    .handler = &proj_fuel,
};

}  // namespace GB::commands
