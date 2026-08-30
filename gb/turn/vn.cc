// SPDX-License-Identifier: Apache-2.0

/// \file vn.cc
/// \brief Assorted Von Neumann machine code.

module;

#include <cstdlib>

import std;
#undef stdout

module gblib;

/// \brief Finds the closest and second-closest star systems to the specified
/// coordinates, excluding the current star system.
///
/// \param em Entity manager for accessing star and universe entities.
/// \param current_star Star system currently orbited (which will be excluded
/// from results).
/// \param xpos X coordinate in universe space.
/// \param ypos Y coordinate in universe space.
/// \return StarTargetResult containing the closest and second-closest
/// starnum_t.
StarTargetResult find_closest_stars(EntityManager& em, starnum_t current_star,
                                    double xpos, double ypos) {
  const auto& universe = *em.peek_universe();
  if (universe.numstars <= 1) {
    return StarTargetResult{.closest = current_star,
                            .second_closest = current_star};
  }

  std::optional<starnum_t> min1;
  std::optional<starnum_t> min2;
  double dist1 = std::numeric_limits<double>::max();
  double dist2 = std::numeric_limits<double>::max();

  // Scan all stars using readonly StarList iteration to track the top two
  // closest candidate systems excluding the ship's current star.
  for (const Star& star : StarList::readonly(em)) {
    if (star.star_id() == current_star) {
      continue;
    }

    const double d = std::hypot(star.xpos() - xpos, star.ypos() - ypos);

    // If closer than the closest star, push the previous closest down to
    // second-closest.
    if (d < dist1) {
      dist2 = dist1;
      min2 = min1;
      dist1 = d;
      min1 = star.star_id();
    } else if (d < dist2) {
      // Otherwise check if closer than the current second-closest.
      dist2 = d;
      min2 = star.star_id();
    }
  }

  const starnum_t closest = min1.value_or(current_star);
  const starnum_t second_closest = min2.value_or(closest);
  return StarTargetResult{.closest = closest, .second_closest = second_closest};
}

/// \brief Selects and assigns a destination target for an autonomous berserker
/// ship.
///
/// If an offending player target is set in TurnStats, the berserker routes
/// toward one of the target player's known star systems; otherwise, it selects
/// a random star.
///
/// \param em Entity manager for entity queries and mutations.
/// \param ship Autonomous berserker ship to assign orders to.
/// \param stats Turn statistics containing aggression tracking (most_mad).
void select_berserker_destination(EntityManager& em, AutonomousShip& ship,
                                  const TurnStats& stats) {
  ship.bombard() = true;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;

  ship.mind().target = stats.VN_brain.most_mad;
  const auto target = ship.mind().target;

  const auto& universe = *em.peek_universe();

  // Route toward the offending player if valid, flipping a coin between
  // primary and secondary target stars recorded in the universe index.
  if (is_valid_player(target)) {
    ship.deststar() =
        bool_rand() ? universe.VN_index1[target] : universe.VN_index2[target];
  } else {
    ship.deststar() = int_rand(0, universe.numstars - 1);
  }

  const auto& star = *em.peek_star(ship.deststar());
  if (auto pnum = star.get_random_planet_index()) {
    ship.destpnum() = *pnum;
  } else {
    ship.destpnum() = 0;
    ship.whatdest() = ScopeLevel::LEVEL_STAR;
  }

  if (ship.hyper_drive().has && ship.mounted()) {
    ship.hyper_drive().on = true;
    ship.hyper_drive().charge = HYPER_DRIVE_READY_CHARGE;
    ship.set_busy(true);
  }
}

/// \brief Selects and assigns a destination target for an autonomous Von
/// Neumann machine.
///
/// Identifies nearest star systems and routes to uninhabited systems, avoiding
/// stars already occupied by Player 1.
///
/// \param em Entity manager for entity queries and mutations.
/// \param ship Autonomous Von Neumann machine to assign orders to.
void select_vn_destination(EntityManager& em, AutonomousShip& ship) {
  const auto& universe = *em.peek_universe();

  auto [closest, second_closest] =
      find_closest_stars(em, ship.storbits(), ship.xpos(), ship.ypos());

  const auto& star_min = *em.peek_star(closest);
  const auto& star_min2 = *em.peek_star(second_closest);

  // Avoid stars already occupied by VN (Player 1); if both nearest are
  // occupied, pick a random star.
  if (star_min.is_inhabited_by(player_t{1})) {
    if (star_min2.is_inhabited_by(player_t{1})) {
      ship.deststar() = int_rand(0, universe.numstars - 1);
    } else {
      ship.deststar() = second_closest;
    }
  } else {
    ship.deststar() = closest;
  }

  const auto& dest_star = *em.peek_star(ship.deststar());
  if (auto pnum = dest_star.get_random_planet_index()) {
    ship.destpnum() = *pnum;
    ship.whatdest() = ScopeLevel::LEVEL_PLAN;
    ship.set_busy(true);
  } else {
    ship.destpnum() = 0;
    ship.whatdest() = ScopeLevel::LEVEL_STAR;
    ship.set_busy(false);
  }
  ship.speed() = Shipdata[ShipType::OTYPE_VN][ABIL_SPEED];
}

namespace {
void order_berserker(EntityManager& em, Ship& ship, TurnStats& stats) {
  if (auto* auto_ship = ship.as<AutonomousShip>()) {
    select_berserker_destination(em, *auto_ship, stats);
  }
}

void order_VN(EntityManager& em, Ship& ship) {
  if (auto* auto_ship = ship.as<AutonomousShip>()) {
    select_vn_destination(em, *auto_ship);
  }
}
}  // namespace

std::optional<player_t>
select_victim_to_steal_from(const Planet& planet,
                            std::span<const player_t> race_order) {
  for (player_t candidate : race_order) {
    if (planet.info(candidate).resource > 0) {
      return candidate;
    }
  }
  return std::nullopt;
}

/// \brief Steals resources from landed non-Player-1 colony stockpiles.
///
/// \param em Entity manager for entity queries, mutations, and messaging.
/// \param ship Autonomous ship performing the theft.
/// \return StealResult containing victim ID and quantity stolen.
StealResult steal_planetary_resources(EntityManager& em, AutonomousShip& ship) {
  auto candidate_ids = shuffled_indices(1, em.num_races().value + 1);
  std::vector<player_t> race_order;
  race_order.reserve(candidate_ids.size());
  for (int id : candidate_ids) {
    race_order.push_back(player_t{id});
  }

  resource_t prod = 0;
  player_t f = 0;
  em.mutate_planet(ship.storbits(), ship.pnumorbits(), [&](Planet& planet_mut) {
    auto victim = select_victim_to_steal_from(planet_mut, race_order);
    if (!victim) return;
    f = *victim;
    prod = std::min(
        planet_mut.info(f).resource,
        static_cast<resource_t>(Shipdata[ShipType::OTYPE_VN][ABIL_COST]));
    planet_mut.info(f).resource -= prod;
  });

  if (f == 0) return StealResult{};

  std::string buf;
  if (ship.type() == ShipType::OTYPE_VN) {
    rcv_resource(ship, static_cast<int>(prod));
    buf = std::format("{0} resources stolen from [{1}] by {2}{3} at {4}.", prod,
                      f, Shipltrs[ShipType::OTYPE_VN], ship.number(),
                      prin_ship_orbits(em, ship));
  } else if (ship.type() == ShipType::OTYPE_BERS) {
    rcv_destruct(ship, static_cast<int>(prod));
    buf = std::format("{0} resources stolen from [{1}] by {2}{3} at {4}.", prod,
                      f, Shipltrs[ShipType::OTYPE_BERS], ship.number(),
                      prin_ship_orbits(em, ship));
  }

  push_telegram_race(em, f, buf);
  if (f != ship.owner()) push_telegram(em, ship.owner(), ship.governor(), buf);
  return StealResult{.victim = f, .amount = prod};
}

/// \brief Mines resources and fuel from the currently occupied sector.
///
/// \param ship Autonomous ship mining the sector.
/// \param sector Sector being mined.
/// \return Quantity of resources extracted from the sector.
resource_t mine_sector(AutonomousShip& ship, Sector& sector) {
  const resource_t oldres = sector.get_resource();
  if (oldres <= 0) {
    return 0;
  }

  sector.set_resource(static_cast<resource_t>(oldres * VN_RES_TAKE));
  const resource_t prod = oldres - sector.get_resource();
  if (ship.type() == ShipType::OTYPE_VN) {
    rcv_resource(ship, static_cast<int>(prod));
  } else if (ship.type() == ShipType::OTYPE_BERS) {
    rcv_destruct(ship, static_cast<int>(5 * prod));
  }
  rcv_fuel(ship, static_cast<double>(prod));
  return prod;
}

/// \brief Moves an autonomous machine to an adjacent sector when current
/// sector is depleted.
///
/// \param ship Autonomous ship to move.
/// \param planet Planet being explored.
/// \return New landed coordinates on the planet.
Coordinates roam_to_adjacent_sector(AutonomousShip& ship,
                                    const Planet& planet) {
  const Coordinates new_coords =
      planet.random_adjacent_coordinates(ship.land_coords());
  ship.set_land_coords(new_coords);
  return new_coords;
}

/*  do_VN() -- called by doship() */
void do_VN(EntityManager& em, Ship& ship, TurnStats& stats) {
  auto* auto_ship = ship.as<AutonomousShip>();
  if (!auto_ship) {
    return;
  }

  if (!auto_ship->is_landed()) {
    if (!auto_ship->mind().busy) {
      return;
    }

    // we were just built & launched
    if (auto_ship->type() == ShipType::OTYPE_BERS)
      order_berserker(em, ship, stats);
    else
      order_VN(em, ship);
    return;
  }

  stats.Stinfo[auto_ship->storbits().value][auto_ship->pnumorbits().value]
      .inhab = true;

  /* launch if no assignment */
  if (!auto_ship->mind().busy) {
    if (auto_ship->fuel() >=
        static_cast<double>(auto_ship->max_fuel_capacity())) {
      const auto& star = *em.peek_star(auto_ship->storbits());
      const auto& planet =
          *em.peek_planet(auto_ship->storbits(), auto_ship->pnumorbits());
      auto_ship->xpos() = star.xpos() + planet.xpos() + int_rand(-10, 10);
      auto_ship->ypos() = star.ypos() + planet.ypos() + int_rand(-10, 10);
      auto_ship->docked() = 0;
      auto_ship->whatdest() = ScopeLevel::LEVEL_UNIV;
    }
    return;
  }

  /* we have an assignment. Since we are landed, this means we are engaged in
     building up resources/fuel. */
  steal_planetary_resources(em, *auto_ship);
}

/// \brief Generates a random binary name (e.g. "01101") for a new Von Neumann
/// machine.
///
/// In Galactic Bloodshed, ship names are non-unique cosmetic designations.
/// Uniqueness is guaranteed by the unique shipnum_t assigned during creation.
///
/// \return std::string of random binary digits ('0' or '1').
std::string generate_vn_binary_name() {
  const int len = int_rand(3, std::min(10, SHIP_NAMESIZE));
  std::string name;
  name.reserve(len);
  for (int i = 0; i < len; ++i) {
    name.push_back(bool_rand() ? '1' : '0');
  }
  return name;
}

/// \brief Constructs and deploys a newly replicated Von Neumann machine on a
/// planet.
///
/// \param em Entity manager for entity persistence.
/// \param parent Autonomous parent machine providing resources, lineage, and
/// fuel.
/// \param planet Planet where replication is occurring.
/// \return Ship number of the newly constructed machine.
shipnum_t construct_replicated_vn(EntityManager& em, AutonomousShip& parent,
                                  Planet& planet) {
  const int cost = Shipdata[ShipType::OTYPE_VN][ABIL_COST];
  use_resource(parent, cost);

  MindData child_mind{
      .progenitor = parent.mind().progenitor,
      .target = parent.mind().target,
      .generation = parent.mind().generation + 1,
      .busy = false,
      .tampered = parent.mind().tampered,
      .who_killed = parent.mind().who_killed,
  };

  ship_struct s2_data{
      .owner = 1,
      .governor = 0,
      .name = generate_vn_binary_name(),
      .xpos = parent.xpos(),
      .ypos = parent.ypos(),
      .fuel = 0.5 * parent.fuel(),
      .land_coords = parent.land_coords(),
      .nextship = planet.ships(),
      .armor = parent.armor() + 1,
      .max_crew = Shipdata[ShipType::OTYPE_VN][ABIL_MAXCREW],
      .max_resource = Shipdata[ShipType::OTYPE_VN][ABIL_CARGO],
      .max_destruct = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_VN][ABIL_DESTCAP]),
      .max_fuel = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_VN][ABIL_FUELCAP]),
      .max_speed =
          static_cast<speed_t>(Shipdata[ShipType::OTYPE_VN][ABIL_SPEED]),
      .tech = parent.tech() + 20.0,
      .special = child_mind,
      .storbits = parent.storbits(),
      .deststar = parent.deststar(),
      .destpnum = parent.destpnum(),
      .pnumorbits = parent.pnumorbits(),
      .whatdest = parent.whatdest(),
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_VN,
      .speed = static_cast<speed_t>(Shipdata[ShipType::OTYPE_VN][ABIL_SPEED]),
      .active = true,
      .alive = true,
      .mode = false,
      .bombard = false,
      .docked = true,
  };

  parent.fuel() *= 0.5;

  auto ship_handle = em.create_ship(s2_data);
  Ship& s2 = *ship_handle;
  s2.size() = ship_size(s2);
  s2.base_mass() = getmass(s2);
  s2.mass() = s2.base_mass();

  planet.ships() = s2.number();
  return s2.number();
}

/// \brief Constructs and deploys a newly constructed Berserker warship on a
/// planet.
///
/// \param em Entity manager for entity persistence and notifications.
/// \param parent Autonomous parent machine providing resources, lineage, and
/// fuel.
/// \param planet Planet where construction is occurring.
/// \param stats Turn statistics containing retaliation target data.
/// \return Ship number of the newly constructed Berserker.
shipnum_t construct_replicated_berserker(EntityManager& em,
                                         AutonomousShip& parent, Planet& planet,
                                         const TurnStats& stats) {
  const int cost = Shipdata[ShipType::OTYPE_BERS][ABIL_COST];
  use_resource(parent, cost);

  MindData bers_mind{
      .progenitor = parent.mind().progenitor,
      .target = stats.VN_brain.most_mad,
      .generation = parent.mind().generation,
      .busy = false,
      .tampered = false,
      .who_killed = parent.mind().who_killed,
  };

  ship_struct s2_data{
      .owner = 1,
      .governor = 0,
      .xpos = parent.xpos(),
      .ypos = parent.ypos(),
      .fuel = 5.0 * parent.fuel(),
      .land_coords = parent.land_coords(),
      .nextship = planet.ships(),
      .armor = parent.armor() + 11,
      .max_crew = Shipdata[ShipType::OTYPE_BERS][ABIL_MAXCREW],
      .max_resource = Shipdata[ShipType::OTYPE_BERS][ABIL_CARGO],
      .max_destruct = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_BERS][ABIL_DESTCAP]),
      .max_fuel = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_BERS][ABIL_FUELCAP]),
      .max_speed =
          static_cast<speed_t>(Shipdata[ShipType::OTYPE_BERS][ABIL_SPEED]),
      .tech = parent.tech() + 100.0,
      .destruct = 500,
      .special = bers_mind,
      .protect = ProtectData{.planet = true, .self = true},
      .hyper_drive = HyperDriveData{.charge = HYPER_DRIVE_READY_CHARGE,
                                    .on = true,
                                    .has = true},
      .storbits = parent.storbits(),
      .deststar = parent.deststar(),
      .destpnum = parent.destpnum(),
      .pnumorbits = parent.pnumorbits(),
      .whatdest = parent.whatdest(),
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .retaliate = static_cast<weapon_power_t>(
          Shipdata[ShipType::OTYPE_BERS][ABIL_GUNS]),
      .type = ShipType::OTYPE_BERS,
      .speed = static_cast<speed_t>(Shipdata[ShipType::OTYPE_BERS][ABIL_SPEED]),
      .active = true,
      .alive = true,
      .mode = false,
      .bombard = true,
      .mounted = true,
      .docked = true,
      .guns = static_cast<gun_count_t>(
          Shipdata[ShipType::OTYPE_BERS][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE),
      .primary = static_cast<weapon_power_t>(
          Shipdata[ShipType::OTYPE_BERS][ABIL_GUNS]),
      .primtype = shipdata_primary(ShipType::OTYPE_BERS),
      .secondary = 0,
      .sectype = shipdata_secondary(ShipType::OTYPE_BERS),
  };

  parent.fuel() *= 0.5;

  auto ship_handle = em.create_ship(s2_data);
  Ship& s2 = *ship_handle;
  s2.size() = ship_size(s2);
  s2.base_mass() = getmass(s2);
  s2.mass() = s2.base_mass();

  planet.ships() = s2.number();

  auto buf =
      std::format("{0} constructed {1}.", static_cast<const Ship&>(parent), s2);
  push_telegram(em, parent.owner(), parent.governor(), buf);
  return s2.number();
}

/// \brief Replicates as many autonomous machines as parent resources allow.
///
/// \param em Entity manager for entity persistence and notifications.
/// \param parent Autonomous machine attempting replication.
/// \param planet Planet where replication is occurring.
/// \param stats Turn statistics for aggression tracking.
/// \return Total number of new machines constructed.
int replicate_machines(EntityManager& em, AutonomousShip& parent,
                       Planet& planet, const TurnStats& stats) {
  const ShipType shipbuild = (stats.VN_brain.total_mad > 100 && bool_rand())
                                 ? ShipType::OTYPE_BERS
                                 : ShipType::OTYPE_VN;
  const int cost = Shipdata[shipbuild][ABIL_COST];
  if (cost <= 0 || parent.resource() < cost) {
    return 0;
  }

  const int count = static_cast<int>(parent.resource() / cost);
  for (int i = 0; i < count; ++i) {
    if (shipbuild == ShipType::OTYPE_BERS) {
      construct_replicated_berserker(em, parent, planet, stats);
    } else {
      construct_replicated_vn(em, parent, planet);
    }
    parent.mind().busy = bool_rand();
  }
  return count;
}

/*  planet_doVN() -- called by doplanet() */
void planet_doVN(Ship& ship, Planet& planet, SectorMap& smap,
                 EntityManager& entity_manager, TurnStats& stats) {
  auto* auto_ship = ship.as<AutonomousShip>();
  if (!auto_ship) {
    return;
  }

  if (auto_ship->is_landed()) {
    if (auto_ship->type() == ShipType::OTYPE_VN && auto_ship->mind().busy) {
      /* first try and make some resources(VNs) by ourselves.
         more might be stolen in doship */
      auto& s = smap.get(auto_ship->land_coords());
      if (s.get_resource() == 0) {
        /* move to another sector */
        roam_to_adjacent_sector(*auto_ship, planet);
      } else {
        /* mine the sector */
        mine_sector(*auto_ship, s);
      }
      replicate_machines(entity_manager, *auto_ship, planet, stats);
    }
  } else { /* orbiting a planet */
    if (std::holds_alternative<MindData>(ship.special()) &&
        std::get<MindData>(ship.special()).busy) {
      if (ship.whatdest() == ScopeLevel::LEVEL_PLAN &&
          ship.deststar() == ship.storbits() &&
          ship.destpnum() == ship.pnumorbits()) {
        if (planet.type() == PlanetType::GASGIANT) {
          if (std::holds_alternative<MindData>(ship.special())) {
            auto mind = std::get<MindData>(ship.special());
            mind.busy = 0;
            ship.special() = mind;
          }
        } else {
          /* find a place on the planet to land */
          bool found = false;
          for (Sector& sect : smap.shuffle()) {
            if (sect.get_resource() == 0) continue;
            found = true;
            ship.docked() = 1;
            ship.whatdest() = ScopeLevel::LEVEL_PLAN;
            ship.deststar() = ship.storbits();
            ship.destpnum() = ship.pnumorbits();
            const auto& star = *entity_manager.peek_star(ship.storbits());
            ship.xpos() = star.xpos() + planet.xpos();
            ship.ypos() = star.ypos() + planet.ypos();
            ship.set_land_coords(sect.coords());
            if (std::holds_alternative<MindData>(ship.special())) {
              auto mind = std::get<MindData>(ship.special());
              mind.busy = 1;
              ship.special() = mind;
            }
            break;
          }
          if (!found && std::holds_alternative<MindData>(ship.special())) {
            auto mind = std::get<MindData>(ship.special());
            mind.busy = 0;
            ship.special() = mind;
          }
        }
      }
    }
  }
}
