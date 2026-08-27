// SPDX-License-Identifier: Apache-2.0

module;
#include <cstdlib>

import std;
import gblib;
import tabulate;

module gblib;

std::expected<char, GroundMovementError> get_ground_order(const Ship& ship,
                                                          std::size_t index) {
  if (!std::holds_alternative<TerraformData>(ship.special())) {
    return std::unexpected(GroundMovementError::NotTerraformVehicle);
  }
  const auto& orders = ship.shipclass();
  if (orders.empty()) {
    return std::unexpected(GroundMovementError::EmptyOrders);
  }
  if (index >= orders.size()) {
    return std::unexpected(GroundMovementError::InvalidIndex);
  }
  const char order = orders[index];
  if (order == '\0') {
    return std::unexpected(GroundMovementError::EmptyOrders);
  }
  if (order == 's') {
    return std::unexpected(GroundMovementError::Stopped);
  }
  return order;
}

std::expected<Coordinates, GroundMovementError>
advance_ground_vehicle(Ship& ship, const Planet& planet,
                       EntityManager& entity_manager) {
  if (!std::holds_alternative<TerraformData>(ship.special())) {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::NotTerraformVehicle);
  }

  auto terraform = std::get<TerraformData>(ship.special());
  if (ship.shipclass().empty()) {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::EmptyOrders);
  }
  if (terraform.index >= ship.shipclass().size()) {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::InvalidIndex);
  }
  if (ship.shipclass()[terraform.index] == 's') {
    ship.on() = 0;
    return std::unexpected(GroundMovementError::Stopped);
  }
  if (ship.shipclass()[terraform.index] == 'c') {
    terraform.index = 0; /* reset the orders */
    ship.special() = terraform;
    if (ship.shipclass().empty() || ship.shipclass()[0] == 'c') {
      ship.on() = 0;
      return std::unexpected(GroundMovementError::EmptyOrders);
    }
    if (ship.shipclass()[0] == 's') {
      ship.on() = 0;
      return std::unexpected(GroundMovementError::Stopped);
    }
  }

  const char order = ship.shipclass()[terraform.index];
  auto [x, y] = get_move(planet, order, ship.land_coords());

  bool bounced = false;

  if (y >= planet.Maxy()) {
    bounced = true;
    y -= 2; /* bounce off of south pole! */
  } else if (y < 0) {
    y = 1;
    bounced = true; /* bounce off of north pole! */
  }
  if (planet.Maxy() == 1) y = 0;

  if (terraform.index + 1 < ship.shipclass().size() &&
      ship.shipclass()[terraform.index + 1] != '\0') {
    ++terraform.index;
    if ((terraform.index + 1 >= ship.shipclass().size() ||
         ship.shipclass()[terraform.index + 1] == '\0') &&
        (!ship.notified())) {
      ship.notified() = 1;
      const std::string teleg_buf =
          std::format("%{0} is out of orders at %{1}.", ship,
                      prin_ship_orbits(entity_manager, ship));
      push_telegram(entity_manager, ship.owner(), ship.governor(), teleg_buf);
    }
    ship.special() = terraform;
  } else if (bounced) {
    if (terraform.index < ship.shipclass().size()) {
      ship.shipclass()[terraform.index] +=
          ((ship.shipclass()[terraform.index] > '5') ? -6 : 6);
    }
  }
  ship.set_land_coords({x, y});
  return Coordinates{x, y};
}

bool moveship_onplanet(Ship& ship, const Planet& planet,
                       EntityManager& entity_manager) {
  return advance_ground_vehicle(ship, planet, entity_manager).has_value();
}

std::expected<bool, GroundActionError>
execute_terraforming(Ship& ship, Planet& planet, SectorMap& smap,
                     EntityManager& entity_manager) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (!ship.popn()) return std::unexpected(GroundActionError::NoCrew);
  if (ship.fuel() < static_cast<double>(FUEL_COST_TERRA)) {
    if (!ship.notified()) {
      ship.notified() = 1;
      msg_OOF(entity_manager, ship);
    }
    return std::unexpected(GroundActionError::InsufficientFuel);
  }

  if (!moveship_onplanet(ship, planet, entity_manager)) {
    return std::unexpected(GroundActionError::MovementFailed);
  }

  auto& s = smap.get(ship.land_coords());
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return std::unexpected(GroundActionError::IncompatibleSector);

  if (s.get_condition() == race->likesbest) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" T{} is full of zealots!!!", ship.number()));
    return std::unexpected(GroundActionError::SectorAlreadyOptimal);
  }

  if (s.get_condition() == SectorType::SEC_GAS) {
    push_telegram(
        entity_manager, ship.owner(), ship.governor(),
        std::format(" T{} is trying to terraform gas.", ship.number()));
    return std::unexpected(GroundActionError::IncompatibleSector);
  }

  const int chance =
      (100 - static_cast<int>(ship.damage())) * ship.popn() / ship.max_crew();
  if (success(chance)) {
    /* only condition can be terraformed, type doesn't change */
    s.terraform(race->likesbest);
    use_fuel(ship, FUEL_COST_TERRA);
    if (success(50) && (planet.conditions(TOXIC) < 100)) {
      planet.conditions(TOXIC) += 1;
    }
    if ((ship.fuel() < static_cast<double>(FUEL_COST_TERRA)) &&
        (!ship.notified())) {
      ship.notified() = 1;
      msg_OOF(entity_manager, ship);
    }
    return true;
  }
  return false;
}

std::expected<int, GroundActionError>
execute_plowing(Ship& ship, Planet& planet, SectorMap& smap,
                EntityManager& entity_manager) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (ship.fuel() < static_cast<double>(FUEL_COST_PLOW)) {
    if (!ship.notified()) {
      ship.notified() = 1;
      msg_OOF(entity_manager, ship);
    }
    return std::unexpected(GroundActionError::InsufficientFuel);
  }

  if (!moveship_onplanet(ship, planet, entity_manager)) {
    return std::unexpected(GroundActionError::MovementFailed);
  }

  auto& s = smap.get(ship.land_coords());
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race || !race->likes[s.get_condition()]) {
    return std::unexpected(GroundActionError::IncompatibleSector);
  }
  if (s.get_fert() >= 100) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" K{} is full of zealots!!!", ship.number()));
    return std::unexpected(GroundActionError::SectorAlreadyOptimal);
  }

  int adjust = round_rand(10 *
                          (0.01 * (100.0 - static_cast<double>(ship.damage())) *
                           static_cast<double>(ship.popn())) /
                          ship.max_crew());
  s.set_fert(std::min(100U, s.get_fert() + adjust));
  if (s.get_fert() >= 100) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" K{} is full of zealots!!!", ship.number()));
  }
  use_fuel(ship, FUEL_COST_PLOW);
  if (success(50) && (planet.conditions(TOXIC) < 100)) {
    planet.conditions(TOXIC) += 1;
  }
  return adjust;
}

std::expected<int, GroundActionError>
upgrade_sector_dome(EntityManager& entity_manager, Ship& ship,
                    SectorMap& smap) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (ship.resource() < RES_COST_DOME) {
    return std::unexpected(GroundActionError::InsufficientResources);
  }

  auto& s = smap.get(ship.land_coords());
  if (s.get_eff() >= 100) {
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  std::format(" Y{} is full of zealots!!!", ship.number()));
    return std::unexpected(GroundActionError::SectorAlreadyOptimal);
  }
  int adjust = round_rand(0.05 * (100.0 - static_cast<double>(ship.damage())) *
                          static_cast<double>(ship.popn()) / ship.max_crew());
  s.improve_efficiency(adjust);
  use_resource(ship, RES_COST_DOME);
  return adjust;
}

std::expected<int, GroundActionError>
strip_mine_quarry(Ship& ship, Planet& planet, SectorMap& smap,
                  EntityManager& entity_manager, TurnStats& stats) {
  if (!ship.on()) return std::unexpected(GroundActionError::NotSwitchedOn);
  if (!landed(ship)) return std::unexpected(GroundActionError::NotLanded);
  if (!ship.popn()) return std::unexpected(GroundActionError::NoCrew);
  if (ship.fuel() < static_cast<double>(FUEL_COST_QUARRY)) {
    if (!ship.notified()) {
      msg_OOF(entity_manager, ship);
      ship.notified() = 1;
    }
    ship.on() = 0;
    return std::unexpected(GroundActionError::InsufficientFuel);
  }

  auto& s = smap.get(ship.land_coords());
  /* nuke the sector */
  s.set_condition(SectorType::SEC_WASTED);
  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return std::unexpected(GroundActionError::IncompatibleSector);

  int prod = round_rand(race->metabolism * static_cast<double>(ship.popn()) /
                        static_cast<double>(ship.max_crew()));
  ship.fuel() -= FUEL_COST_QUARRY;
  stats.prod_res[ship.owner()] += prod;
  int tox = int_rand(0, int_rand(0, prod));
  planet.conditions(TOXIC) = std::min(100, planet.conditions(TOXIC) + tox);
  if (s.get_fert() >= prod) {
    s.set_fert(s.get_fert() - prod);
  } else {
    s.set_fert(0);
  }
  return prod;
}

bool execute_berserker_bombardment(EntityManager& entity_manager, Ship& ship,
                                   Planet& planet) {
  if (ship.whatdest() != ScopeLevel::LEVEL_PLAN ||
      ship.whatorbits() != ScopeLevel::LEVEL_PLAN || landed(ship) ||
      ship.storbits() != ship.deststar() ||
      ship.pnumorbits() != ship.destpnum()) {
    return false;
  }

  const auto* race = entity_manager.peek_race(ship.owner());
  if (!race) return false;

  int destroyed = berserker_bombard(entity_manager, ship, planet, *race);
  if (destroyed == 0) {
    const auto* dest_star = entity_manager.peek_star(ship.storbits());
    ship.destpnum() = int_rand(0, dest_star->numplanets() - 1);
    return false;
  }

  if (std::holds_alternative<MindData>(ship.special())) {
    auto mind = std::get<MindData>(ship.special());
    if (mind.who_killed.value > 0 && mind.who_killed.value <= MAXPLAYERS) {
      auto universe_handle = entity_manager.get_universe();
      if (universe_handle->VN_hitlist[mind.who_killed.value - 1] > 0) {
        --universe_handle->VN_hitlist[mind.who_killed.value - 1];
      }
    }
  }
  return true;
}

double refuel_gasgiant_orbiters(const Planet& planet, Ship& ship) {
  if (landed(ship) || planet.type() != PlanetType::GASGIANT) {
    return 0.0;
  }

  double fadd = 0.0;
  switch (ship.type()) {
    case ShipType::STYPE_TANKER:
      fadd = FUEL_GAS_ADD_TANKER;
      break;
    case ShipType::STYPE_HABITAT:
      fadd = FUEL_GAS_ADD_HABITAT;
      break;
    default:
      fadd = FUEL_GAS_ADD;
      break;
  }
  const double capacity = static_cast<double>(max_fuel(ship)) - ship.fuel();
  const double added = std::clamp(fadd, 0.0, std::max(0.0, capacity));
  if (added > 0.0) {
    rcv_fuel(ship, added);
  }
  return added;
}

bool check_mutual_alliances(EntityManager& entity_manager,
                            std::span<const player_t> players) {
  if (players.size() <= 1) {
    return true;
  }
  std::bitset<MAXPLAYERS + 1> required_mask;
  for (const player_t p : players) {
    required_mask.set(p.value);
  }

  return std::ranges::all_of(players, [&](player_t p) {
    const auto* race = entity_manager.peek_race(p);
    std::bitset<MAXPLAYERS + 1> peers_mask = required_mask;
    peers_mask.reset(p.value);
    const std::bitset<MAXPLAYERS + 1> race_allied(race->allied);
    return (race_allied & peers_mask) == peers_mask;
  });
}

std::expected<PlunderDistribution, PlunderError>
calculate_plunder_distribution(Stockpile total_loot,
                               std::span<const player_t> conquerors) {
  if (conquerors.empty()) {
    return std::unexpected(PlunderError::NoConquerors);
  }
  if (total_loot.empty()) {
    return std::unexpected(PlunderError::EmptyLoot);
  }

  const std::size_t shares_count = conquerors.size();
  Stockpile remaining = total_loot;
  std::vector<PlayerLootShare> shares;
  shares.reserve(shares_count);

  for (std::size_t idx = 0; idx + 1 < shares_count; ++idx) {
    const player_t conqueror = conquerors[idx];
    const Stockpile allocated =
        total_loot.split_share(shares_count).clamp_to(remaining);
    remaining -= allocated;
    shares.push_back(PlayerLootShare{.player = conqueror, .share = allocated});
  }

  // Last conqueror gets all leftovers
  shares.push_back(
      PlayerLootShare{.player = conquerors.back(), .share = remaining});

  return PlunderDistribution{
      .shares = std::move(shares),
      .total_loot = total_loot,
  };
}

std::optional<RecoveryReport>
recover_conquered_stockpiles(EntityManager& entity_manager, const Star& star,
                             Planet& planet) {
  std::vector<player_t> owners;
  Stockpile total_stolen;

  for (const Race* race : RaceList::readonly(entity_manager)) {
    if (planet.info(*race).numsectsowned > 0) {
      owners.push_back(race->Playernum);
    } else if (!race->God) { /* Can't steal from God */
      total_stolen += planet.info(*race).stockpile();
    }
  }

  if (!check_mutual_alliances(entity_manager, owners)) {
    return std::nullopt;
  }

  auto distribution = calculate_plunder_distribution(total_stolen, owners);
  if (!distribution.has_value()) {
    return std::nullopt;
  }

  // 1. Deposit plunder into conqueror colonies
  for (const auto& [conqueror, allocated] : distribution->shares) {
    planet.info(conqueror).deposit_stockpile(allocated);
  }

  // 2. Drain stockpiles from defeated non-god races
  for (const Race* race : RaceList::readonly(entity_manager)) {
    if (planet.info(*race).numsectsowned == 0 && !race->God) {
      planet.info(*race).drain_stockpile();
    }
  }

  const planetnum_t planetnum = planet.planet_order();
  return RecoveryReport{
      .star_id = star.star_id(),
      .star_name = star.get_name(),
      .planet_name = star.get_planet_name(planetnum),
      .planet_num = planetnum,
      .recipients = std::move(owners),
      .allocated_shares = std::move(distribution->shares),
      .total_stolen = total_stolen,
  };
}

std::string format_recovery_report(const RecoveryReport& report,
                                   EntityManager& entity_manager) {
  if (report.empty()) {
    return {};
  }

  std::stringstream out;
  out << std::format("Recovery Report: Planet /{}/{}\n", report.star_name,
                     report.planet_name);

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  table.add_row({"", "res", "destr", "fuel", "xtal"});

  for (const auto& [conqueror, allocated] : report.allocated_shares) {
    const auto* race = entity_manager.peek_race(conqueror);
    table.add_row({
        race ? race->name : std::format("Player {}", conqueror),
        std::format("{}", allocated.resources),
        std::format("{}", allocated.destruct),
        std::format("{}", allocated.fuel),
        std::format("{}", allocated.crystals),
    });
  }

  table.add_row({
      "Total:",
      std::format("{}", report.total_stolen.resources),
      std::format("{}", report.total_stolen.destruct),
      std::format("{}", report.total_stolen.fuel),
      std::format("{}", report.total_stolen.crystals),
  });

  table.column(0).format().font_align(tabulate::FontAlign::left);
  table.column(1).format().font_align(tabulate::FontAlign::right);
  table.column(2).format().font_align(tabulate::FontAlign::right);
  table.column(3).format().font_align(tabulate::FontAlign::right);
  table.column(4).format().font_align(tabulate::FontAlign::right);

  out << table << "\n";
  return out.str();
}

void dispatch_recovery_telegrams(EntityManager& entity_manager,
                                 const Star& star,
                                 const RecoveryReport& report) {
  const auto msg = format_recovery_report(report, entity_manager);
  if (msg.empty()) {
    return;
  }

  for (const player_t recipient : report.recipients) {
    const governor_t gov = star.governor(recipient);
    push_telegram(entity_manager, recipient, gov, msg);
  }
}

void do_recover(EntityManager& entity_manager, const Star& star,
                Planet& planet) {
  const auto report =
      recover_conquered_stockpiles(entity_manager, star, planet);
  if (report.has_value()) {
    dispatch_recovery_telegrams(entity_manager, star, *report);
  }
}

void process_planetary_ships(EntityManager& entity_manager, Planet& planet,
                             SectorMap& smap, TurnStats& stats) {
  for (auto ship_handle : ShipList(entity_manager, planet.ships())) {
    auto& ship = *ship_handle;
    if (ship.alive() && !ship.rad()) {
      /* planet level functions - do these here because they use the sector map
              or affect planet production */
      switch (ship.type()) {
        case ShipType::OTYPE_VN:
          planet_doVN(ship, planet, smap, entity_manager, stats);
          break;
        case ShipType::OTYPE_BERS:
          if (!ship.destruct() || !ship.bombard())
            planet_doVN(ship, planet, smap, entity_manager, stats);
          else
            execute_berserker_bombardment(entity_manager, ship, planet);
          break;
        case ShipType::OTYPE_TERRA:
          execute_terraforming(ship, planet, smap, entity_manager);
          break;
        case ShipType::OTYPE_PLOW: {
          auto plow_res = execute_plowing(ship, planet, smap, entity_manager);
          if (!plow_res) {
            if (plow_res.error() == GroundActionError::NotLanded) {
              push_telegram(entity_manager, ship.owner(), ship.governor(),
                            std::format("K{} is not landed.", ship.number()));
            } else if (plow_res.error() == GroundActionError::NotSwitchedOn) {
              push_telegram(
                  entity_manager, ship.owner(), ship.governor(),
                  std::format("K{} is not switched on.", ship.number()));
            }
          }
          break;
        }
        case ShipType::OTYPE_DOME: {
          auto dome_res = upgrade_sector_dome(entity_manager, ship, smap);
          if (!dome_res) {
            if (dome_res.error() == GroundActionError::InsufficientResources) {
              push_telegram(entity_manager, ship.owner(), ship.governor(),
                            std::format("Y{} does not have enough resources.",
                                        ship.number()));
            } else if (dome_res.error() == GroundActionError::NotLanded) {
              push_telegram(entity_manager, ship.owner(), ship.governor(),
                            std::format("Y{} is not landed.", ship.number()));
            } else if (dome_res.error() == GroundActionError::NotSwitchedOn) {
              push_telegram(
                  entity_manager, ship.owner(), ship.governor(),
                  std::format("Y{} is not switched on.", ship.number()));
            }
          }
          break;
        }
        case ShipType::OTYPE_WPLANT:
          if (landed(ship))
            if (ship.resource() >= RES_COST_WPLANT &&
                ship.fuel() >= FUEL_COST_WPLANT)
              stats.prod_destruct[ship.owner()] +=
                  do_weapon_plant(ship, entity_manager);
            else {
              if (ship.resource() < RES_COST_WPLANT) {
                std::string buf = std::format(
                    "W{} does not have enough resources.", ship.number());
                push_telegram(entity_manager, ship.owner(), ship.governor(),
                              buf);
              } else {
                std::string buf = std::format("W{} does not have enough fuel.",
                                              ship.number());
                push_telegram(entity_manager, ship.owner(), ship.governor(),
                              buf);
              }
            }
          else {
            std::string buf = std::format("W{} is not landed.", ship.number());
            push_telegram(entity_manager, ship.owner(), ship.governor(), buf);
          }
          break;
        case ShipType::OTYPE_QUARRY: {
          auto quarry_res =
              strip_mine_quarry(ship, planet, smap, entity_manager, stats);
          if (!quarry_res) {
            std::string buf;
            if (quarry_res.error() == GroundActionError::NotSwitchedOn) {
              buf = std::format("q{} is not switched on.", ship.number());
            } else if (quarry_res.error() == GroundActionError::NotLanded) {
              buf = std::format("q{} is not landed.", ship.number());
            } else if (quarry_res.error() == GroundActionError::NoCrew) {
              buf = std::format("q{} does not have workers aboard.",
                                ship.number());
            }
            if (!buf.empty()) {
              push_telegram(entity_manager, ship.owner(), ship.governor(), buf);
            }
          }
          break;
        }
        default:
          break;
      }
      /* add fuel for ships orbiting a gas giant */
      refuel_gasgiant_orbiters(planet, ship);
    }
  }
}

void process_planet_climate(Planet& planet, const Star& star,
                            const TurnStats& stats) {
  const starnum_t starnum = star.star_id();
  const planetnum_t planetnum = planet.planet_order();
  planet.update_climate(stats.Stinfo[starnum.value][planetnum.value].temp_add);
}

std::optional<Coordinates>
process_toxic_environmental_damage(const Planet& planet, SectorMap& smap) {
  if (planet.conditions(TOXIC) <= ENVIR_DAMAGE_TOX) {
    return std::nullopt;
  }
  auto& p = smap.get_random();
  p.devastate();
  return p.coords();
}

bool process_supernova_sector_devastation(const Star& star, SectorMap& smap) {
  if (!star.nova_stage()) {
    return false;
  }
  bool affected = false;
  for (Sector& p : smap.occupied()) {
    p.apply_supernova(star.nova_stage());
    affected = true;
  }
  return affected;
}

std::optional<shipnum_t>
build_automated_waste_can(EntityManager& entity_manager, const Star& star,
                          Planet& planet, SectorMap& smap, const Race& race) {
  auto& info = planet.info(race);
  if (!info.tox_thresh.has_value() ||
      planet.conditions(TOXIC) < *info.tox_thresh ||
      info.resource < Shipcost(ShipType::OTYPE_TOXWC, race)) {
    return std::nullopt;
  }

  const int t = std::min(TOXMAX, planet.conditions(TOXIC));
  planet.conditions(TOXIC) -= t;

  const starnum_t starnum = star.star_id();
  const planetnum_t planetnum = planet.planet_order();
  const player_t player = race.Playernum;

  ship_struct s2{
      .owner = player,
      .governor = star.governor(player),
      .xpos = star.xpos() + planet.xpos(),
      .ypos = star.ypos() + planet.ypos(),
      .mass = 1.0,
      .land_coords = smap.get_random().coords(),
      .armor = static_cast<unsigned char>(
          Shipdata[ShipType::OTYPE_TOXWC][ABIL_ARMOR]),
      .max_crew = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_TOXWC][ABIL_MAXCREW]),
      .max_resource =
          static_cast<resource_t>(Shipdata[ShipType::OTYPE_TOXWC][ABIL_CARGO]),
      .max_destruct = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_TOXWC][ABIL_DESTCAP]),
      .max_fuel = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_TOXWC][ABIL_FUELCAP]),
      .max_speed = static_cast<unsigned short>(
          Shipdata[ShipType::OTYPE_TOXWC][ABIL_SPEED]),
      .build_cost =
          static_cast<unsigned short>(Shipcost(ShipType::OTYPE_TOXWC, race)),
      .base_mass = 1.0,
      .special = WasteData{.toxic = static_cast<unsigned char>(t)},
      .storbits = starnum,
      .deststar = starnum,
      .destpnum = planetnum,
      .pnumorbits = planetnum,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_TOXWC,
      .active = 1,
      .alive = 1,
      .docked = 1,
      .guns = GTYPE_NONE,
      .primary = static_cast<unsigned long>(
          Shipdata[ShipType::OTYPE_TOXWC][ABIL_GUNS]),
      .primtype = shipdata_primary(ShipType::OTYPE_TOXWC),
      .sectype = shipdata_secondary(ShipType::OTYPE_TOXWC),
  };
  auto ship_handle = entity_manager.create_ship(s2);
  Ship& ship = *ship_handle;
  ship.name() = std::format("Scum{:04d}", ship.number());
  ship.size() = ship_size(ship);

  insert_sh_plan(planet, &ship);
  return ship.number();
}

double est_production(const Sector& s, EntityManager& entity_manager) {
  const auto* race = entity_manager.peek_race(s.get_owner());
  return (race->metabolism * (double)s.get_eff() * (double)s.get_eff() / 200.0);
}

std::optional<IslandDiscovery>
process_island_exploration(EntityManager& entity_manager, const Star& star,
                           Planet& planet, SectorMap& smap, TurnStats& stats) {
  if (planet.expltimer() >= 1) {
    planet.expltimer() -= 1;
  }
  if (star.nova_stage() || planet.expltimer() > 0) {
    return std::nullopt;
  }

  planet.expltimer() = 5;
  PlanetExplorationContext exploration{planet};
  bool allexp = false;
  int timer = 20;
  std::optional<IslandDiscovery> discovery;

  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    if (stats.Claims || allexp) {
      break;
    }
    if (planet.info(p).numsectsowned == 0) {
      continue;
    }

    while (!stats.Claims && !allexp && timer > 0) {
      timer -= 1;
      bool all_sectors_explored_for_player = true;
      for (Sector& s : smap.shuffle()) {
        all_sectors_explored_for_player &=
            exploration.is_explored(s.coords(), p);
        if (exploration.is_explored(s.coords(), p) && success(50) &&
            s.get_owner() == 0 && s.get_condition() != SectorType::SEC_WASTED &&
            s.get_condition() == race->likesbest) {
          stats.Claims = true;
          s.colonize(p, race->number_sexes);
          stats.tot_captured = 1;
          discovery = IslandDiscovery{.coords = s.coords(), .player = p};
          break;
        }
        exploration.explore_sector(s, p);
      }
      allexp = (allexp || all_sectors_explored_for_player);
    }
  }

  if (allexp) {
    planet.expltimer() = 5;
  }

  return discovery;
}

void divert_slave_tribute(EntityManager& entity_manager, Planet& planet,
                          TurnStats& stats, player_t master) {
  auto& master_info = planet.info(master);
  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    if (planet.info(p).numsectsowned > 0) {
      master_info.resource += std::exchange(stats.prod_res[p], 0);
      master_info.fuel += std::exchange(stats.prod_fuel[p], 0);
      master_info.destruct += std::exchange(stats.prod_destruct[p], 0);
    }
  }
}

void notify_slave_revolt(EntityManager& entity_manager, const Star& star,
                         const Planet& planet, player_t former_master) {
  const std::string message = std::format(
      "\nThere has been a SLAVE REVOLT on /{}/{}!\n"
      "All population belonging to player #{} on the planet have been killed!\n"
      "Productions now go to their rightful owners.\n",
      star.get_name(), star.get_planet_name(planet.planet_order()),
      former_master);

  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t r_id = race->Playernum;
    if (planet.info(r_id).numsectsowned > 0) {
      push_telegram(entity_manager, r_id, star.governor(r_id), message);
    }
  }
}

EnslavementResult execute_slave_revolt(EntityManager& entity_manager,
                                       const Star& star, Planet& planet,
                                       SectorMap& smap, bool intimidated) {
  const player_t former_master = planet.slaved_to();
  int collateral_devastated = 0;
  int revolt_sectors = planet.calculate_revolt_devastation_count();
  while (--revolt_sectors) {
    auto& p = smap.get_random();
    if (p.get_popn() + p.get_troops() > 0) {
      p.devastate();
      collateral_devastated++;
    }
  }

  int master_devastated = 0;
  if (intimidated) {
    for (Sector& p : smap.shuffle()) {
      if (p.get_owner() == former_master && success(50)) {
        p.devastate();
        master_devastated++;
      }
    }
  }

  notify_slave_revolt(entity_manager, star, planet, former_master);
  planet.free_slaves();

  return EnslavementResult{
      .outcome = EnslavementOutcome::SlaveRevolt,
      .master = former_master,
      .collateral_devastated_count = collateral_devastated,
      .master_devastated_count = master_devastated,
  };
}

EnslavementResult process_enslavement_and_revolts(EntityManager& entity_manager,
                                                  const Star& star,
                                                  Planet& planet,
                                                  SectorMap& smap,
                                                  TurnStats& stats) {
  if (!planet.is_enslaved()) {
    return EnslavementResult{.outcome = EnslavementOutcome::None};
  }

  const player_t master = planet.slaved_to();
  if (!planet.is_slave_revolt_triggered()) {
    divert_slave_tribute(entity_manager, planet, stats, master);
    return EnslavementResult{
        .outcome = EnslavementOutcome::ProductionDiverted,
        .master = master,
    };
  }

  const bool intimidated =
      stats.Stinfo[star.star_id().value][planet.planet_order().value]
          .intimidated;
  return execute_slave_revolt(entity_manager, star, planet, smap, intimidated);
}

void recalculate_census(EntityManager& entity_manager, const Star& star,
                        Planet& planet, const SectorMap& smap,
                        TurnStats& stats) {
  planet.popn() = 0;
  planet.troops() = 0;
  planet.maxpopn() = 0;
  planet.total_resources() = 0;

  for (const Race* race : RaceList::readonly(entity_manager)) {
    auto& info = planet.info(*race);
    info.numsectsowned = 0;
    info.popn = 0;
    info.troops = 0;
  }

  const auto toxic = planet.conditions(TOXIC);
  const auto star_id = star.star_id().value;

  for (const Sector& s : smap) {
    planet.total_resources() += s.get_resource();
    if (!s.is_owned()) {
      continue;
    }

    const player_t owner = s.get_owner();
    auto& pinfo = planet.info(owner);
    pinfo.numsectsowned++;
    pinfo.troops += s.get_troops();
    pinfo.popn += s.get_popn();
    planet.popn() += s.get_popn();
    planet.troops() += s.get_troops();

    const auto* owner_race = entity_manager.peek_race(owner);
    planet.maxpopn() += maxsupport(*owner_race, s, stats.Compat[owner], toxic);

    auto& power = stats.Power[owner];
    power.troops += s.get_troops();
    power.popn += s.get_popn();
    power.sum_eff += s.get_eff();
    power.sum_mob += s.get_mobilization();
    stats.starpopns[star_id][owner] += s.get_popn();
  }
}

int doplanet(EntityManager& entity_manager, const Star& star, Planet& planet,
             TurnStats& stats) {
  int allmod = 0;

  // Extract indices for array access and ship creation
  const starnum_t starnum = star.star_id();
  const planetnum_t planetnum = planet.planet_order();

  // Reset per-planet state in TurnStats
  stats.Claims = false;

  planet.maxpopn() = 0;

  planet.popn() = 0; /* initialize population for recount */
  planet.troops() = 0;
  planet.total_resources() = 0;

  /* reset global variables */
  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    stats.Compat[p] = planet.compatibility(*race);
    planet.info(p).numsectsowned = 0;
    planet.info(p).troops = 0;
    planet.info(p).popn = 0;
    planet.info(p).est_production = 0.0;
    stats.prod_crystals[p] = 0;
    stats.prod_fuel[p] = 0;
    stats.prod_destruct[p] = 0;
    stats.prod_res[p] = 0;
    stats.avg_mob[p] = 0;
  }

  auto smap_handle = entity_manager.get_sectormap(starnum, planetnum);
  if (!smap_handle.get()) {
    return 0;
  }
  auto& smap = *smap_handle;
  process_planetary_ships(entity_manager, planet, smap, stats);
  process_planet_climate(planet, star, stats);

  if (star.nova_stage()) {
    if (process_supernova_sector_devastation(star, smap)) {
      allmod = 1;
    }
  } else {
    for (Sector& p : smap.shuffle()) {
      if (p.is_occupied()) {
        allmod = 1;
        produce(entity_manager, star, planet, p, stats);
        if (p.is_owned()) {
          planet.info(p.get_owner()).est_production +=
              est_production(p, entity_manager);
        }
        spread(entity_manager, planet, p, smap, stats);
      }
      p.clear_owner_if_empty();
    }
  }

  /*
      if (p->wasted) {
          if (x>1 && x<planet->Maxx-2) {
              if (p->des==DES_SEA || p->des==DES_GAS) {
                  if ( y>1 && y<planet->Maxy-2 &&
                      (!(p-1)->wasted || !(p+1)->wasted) && !random()%5)
                      p->wasted = 0;
              } else if (p->des==DES_LAND || p->des==DES_MOUNT
                         || p->des==DES_ICE) {
                  if ( y>1 && y<planet->Maxy-2 && ((p-1)->popn || (p+1)->popn)
                      && !random()%10)
                      p->wasted = 0;
              }
          }
      }
  */
  /*
      if (entity_manager.peek_star(starnum)->nova_stage) {
          if (p->des==DES_ICE)
              if(random()&01)
                  p->des = DES_LAND;
              else if (p->des==DES_SEA)
                  if(random()&01)
                      if ( (x>0 && (p-1)->des==DES_LAND) ||
                          (x<planet->Maxx-1 && (p+1)->des==DES_LAND) ||
                          (y>0 && (p-planet->Maxx)->des==DES_LAND) ||
                          (y<planet->Maxy-1 && (p+planet->Maxx)->des==DES_LAND
     ) ) {
                          p->des = DES_LAND;
                          p->popn = p->owner = p->troops = 0;
                          p->resource += int_rand(1,5);
                          p->fert = int_rand(1,4);
                      }
                      }
                      */

  for (const auto& p : smap.owned()) {
    planet.info(p.get_owner()).numsectsowned++;
  }

  process_island_exploration(entity_manager, star, planet, smap, stats);

  /* environment nukes a random sector if toxic threshold exceeded */
  const auto envir_damage = process_toxic_environmental_damage(planet, smap);

  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    auto& info = planet.info(*race);
    info.prod_crystals = stats.prod_crystals[p];
    info.prod_res = stats.prod_res[p];
    info.prod_fuel = stats.prod_fuel[p];
    info.prod_dest = stats.prod_destruct[p];
    if (info.autorep) {
      info.autorep--;
      std::stringstream telegram_buf;
      telegram_buf << std::format("\nFrom /{}/{}\n", star.get_name(),
                                  star.get_planet_name(planetnum));

      if (stats.Stinfo[starnum.value][planetnum.value].temp_add) {
        telegram_buf << std::format("Temp: {} to {}\n",
                                    planet.conditions(RTEMP),
                                    planet.conditions(TEMP));
      }
      telegram_buf << std::format("Total      Prod: {}r {}f {}d\n",
                                  stats.prod_res[p], stats.prod_fuel[p],
                                  stats.prod_destruct[p]);
      if (stats.prod_crystals[p]) {
        telegram_buf << std::format("    {} crystals found\n",
                                    stats.prod_crystals[p]);
      }
      if (stats.tot_captured) {
        telegram_buf << std::format("{} sectors captured\n",
                                    stats.tot_captured);
      }
      if (star.nova_stage()) {
        telegram_buf << std::format(
            "This planet's primary is in a Stage {} nova.\n",
            star.nova_stage());
      }
      /* remind the player that he should clean up the environment. */
      if (envir_damage.has_value()) {
        telegram_buf << std::format("Environmental damage on sector {},{}\n",
                                    envir_damage->x, envir_damage->y);
      }
      if (planet.slaved_to() != 0) {
        telegram_buf << std::format("ENSLAVED to player {}\n",
                                    planet.slaved_to());
      }
      push_telegram(entity_manager, p, star.governor(p), telegram_buf.str());
    }
  }

  /* find out who is on this planet, for nova notification */
  if (star.nova_stage() == 1) {
    {
      std::stringstream telegram_buf;
      telegram_buf << std::format("BULLETIN from /{}/{}\n", star.get_name(),
                                  star.get_planet_name(planetnum));
      telegram_buf << std::format("\nStar {} is undergoing nova.\n",
                                  star.get_name());
      if (planet.type() == PlanetType::EARTH ||
          planet.type() == PlanetType::WATER ||
          planet.type() == PlanetType::FOREST) {
        telegram_buf << "Seas and rivers are boiling!\n";
      }
      telegram_buf << "This planet must be evacuated immediately!\n"
                   << TELEG_DELIM;
      for (const Race* race : RaceList::readonly(entity_manager)) {
        const player_t p = race->Playernum;
        if (planet.info(p).numsectsowned) {
          push_telegram(entity_manager, p, star.governor(p),
                        telegram_buf.str());
        }
      }
    }
  }

  do_recover(entity_manager, star, planet);

  recalculate_census(entity_manager, star, planet, smap, stats);

  process_enslavement_and_revolts(entity_manager, star, planet, smap, stats);

  /* add production to all people here */
  for (auto race_handle : RaceList(entity_manager)) {
    auto& race = *race_handle;
    const player_t player = race.Playernum;
    auto& info = planet.info(player);
    if (info.numsectsowned > 0) {
      info.deposit_production(stats.prod_fuel[player], stats.prod_res[player],
                              stats.prod_destruct[player],
                              stats.prod_crystals[player]);

      const auto gov_idx = star.governor(player);
      auto& gov = race.governor[gov_idx.value];

      /* tax the population - set new tax rate when done */
      info.collect_tax(gov, race);

      /* do tech investments */
      info.invest_tech(gov, race);

      /* build wc's if it's been ordered */
      build_automated_waste_can(entity_manager, star, planet, smap, race);
    }
  } /* (if numsectsowned) */

  if (planet.maxpopn() > 0 && planet.conditions(TOXIC) < 100)
    planet.conditions(TOXIC) += planet.popn() / planet.maxpopn();

  if (planet.conditions(TOXIC) > 100)
    planet.conditions(TOXIC) = 100;
  else if (planet.conditions(TOXIC) < 0)
    planet.conditions(TOXIC) = 0;

  for (const Race* race : RaceList::readonly(entity_manager)) {
    const player_t p = race->Playernum;
    auto& info = planet.info(p);
    stats.Power[p].resource += info.resource;
    stats.Power[p].destruct += info.destruct;
    stats.Power[p].fuel += info.fuel;
    stats.Power[p].sectors_owned += info.numsectsowned;
    stats.Power[p].planets_owned += !!info.numsectsowned;
    info.update_combat_readiness(stats.avg_mob[p]);
  }
  return allmod;
}
