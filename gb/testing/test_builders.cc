// SPDX-License-Identifier: Apache-2.0

/// \file test_builders.cc
/// \brief Implementation of TestShipBuilder and TestWorldBuilder fixture
/// builders.

module;

#include <cassert>

module test;

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import std;

void TestShipBuilder::init(ShipType type,
                           std::optional<shipnum_t> explicit_number) {
  auto ship = ShipFactory::create_from_template(type, 1);
  ship_ = ship->to_struct();
  ship_.number = explicit_number.value_or(0);
  ship_.on = true;
  ship_.tech = 100.0;
  ship_.fuel = ship_.max_fuel;
  ship_.destruct = ship_.max_destruct;

  Ship temp_ship{ship_};
  ship_.mass = temp_ship.local_mass(1.0);
}

TestShipBuilder::TestShipBuilder(EntityManager& em, ShipType type,
                                 std::optional<shipnum_t> explicit_number)
    : em_(em) {
  init(type, explicit_number);
}

TestShipBuilder& TestShipBuilder::owned_by(player_t owner, governor_t gov) {
  ship_.owner = owner;
  ship_.governor = gov;
  return *this;
}

TestShipBuilder& TestShipBuilder::named(std::string_view name) {
  ship_.name = name;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_tech(double tech) {
  ship_.tech = tech;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_alive(bool alive) {
  ship_.alive = alive;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_active(bool active) {
  ship_.active = active;
  return *this;
}

TestShipBuilder&
TestShipBuilder::in_star_orbit(starnum_t snum,
                               std::optional<UniverseCoordinates> coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_STAR;
  ship_.storbits = snum;
  ship_.pnumorbits = 0;
  ship_.docked = 0;
  if (coords) {
    ship_.coordinates = *coords;
  } else {
    try {
      const auto* star = em_.peek_star(snum);
      ship_.coordinates = star->coordinates();
    } catch (const EntityNotFoundError&) {
      // Star not present in EntityManager (isolated unit tests)
    }
  }
  return *this;
}

TestShipBuilder& TestShipBuilder::in_star_orbit(starnum_t snum,
                                                SystemCoordinates coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_STAR;
  ship_.storbits = snum;
  ship_.docked = 0;
  try {
    const auto* star = em_.peek_star(snum);
    ship_.coordinates = star->coordinates() + coords;
  } catch (const EntityNotFoundError&) {
    // Star not present in EntityManager (isolated unit tests)
  }
  return *this;
}

TestShipBuilder& TestShipBuilder::in_star_orbit(starnum_t snum, double x,
                                                double y) {
  return in_star_orbit(snum, UniverseCoordinates{x, y});
}

TestShipBuilder&
TestShipBuilder::in_planet_orbit(starnum_t snum, planetnum_t pnum,
                                 std::optional<UniverseCoordinates> coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship_.storbits = snum;
  ship_.pnumorbits = pnum;
  ship_.docked = 0;
  if (coords) {
    ship_.coordinates = *coords;
  } else {
    try {
      const auto* star = em_.peek_star(snum);
      const auto* planet = em_.peek_planet(snum, pnum);
      ship_.coordinates = planet->absolute_coordinates(*star);
    } catch (const EntityNotFoundError&) {
      // Star or planet not present in EntityManager (isolated unit tests)
    }
  }
  return *this;
}

TestShipBuilder& TestShipBuilder::in_planet_orbit(starnum_t snum,
                                                  planetnum_t pnum,
                                                  SystemCoordinates coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship_.storbits = snum;
  ship_.pnumorbits = pnum;
  ship_.docked = 0;
  try {
    const auto* star = em_.peek_star(snum);
    ship_.coordinates = star->coordinates() + coords;
  } catch (const EntityNotFoundError&) {
    // Star not present in EntityManager (isolated unit tests)
  }
  return *this;
}

TestShipBuilder& TestShipBuilder::landed_on(starnum_t snum, planetnum_t pnum,
                                            Coordinates coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship_.whatdest = ScopeLevel::LEVEL_PLAN;
  ship_.storbits = snum;
  ship_.pnumorbits = pnum;
  ship_.docked = 1;
  ship_.land_coords = coords;
  try {
    const auto* star = em_.peek_star(snum);
    const auto* planet = em_.peek_planet(snum, pnum);
    ship_.coordinates = planet->absolute_coordinates(*star);
  } catch (const EntityNotFoundError&) {
    // Star or planet not present in EntityManager (isolated unit tests)
  }
  return *this;
}

TestShipBuilder& TestShipBuilder::docked_to(shipnum_t dest_ship,
                                            starnum_t snum) {
  ship_.whatorbits = ScopeLevel::LEVEL_SHIP;
  ship_.whatdest = ScopeLevel::LEVEL_SHIP;
  ship_.destshipno = dest_ship;
  ship_.storbits = snum;
  ship_.docked = 1;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_guns(guntype_t primtype,
                                            gun_count_t count,
                                            ActiveBattery active_battery) {
  ship_.guns = active_battery;
  ship_.primary_battery = GunBattery::create(count, primtype);
  ship_.retaliate = count;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_retaliate(weapon_power_t retaliate) {
  ship_.retaliate = retaliate;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_cew(weapon_power_t cew_power,
                                           unsigned short range) {
  ship_.cew = cew_power;
  ship_.cew_range = range;
  ship_.mounted = true;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_crew(population_t civilians,
                                            population_t military) {
  ship_.popn = civilians;
  ship_.troops = military;
  ship_.mass = ship_.base_mass + (civilians + military);
  return *this;
}

TestShipBuilder& TestShipBuilder::with_max_crew(population_t max_crew) {
  ship_.max_crew = max_crew;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_speed(speed_t speed) {
  ship_.speed = speed;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_max_speed(speed_t max_speed) {
  ship_.max_speed = max_speed;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_fuel(double fuel) {
  ship_.fuel = fuel;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_max_fuel(double max_fuel) {
  ship_.max_fuel = max_fuel;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_resource(resource_t res) {
  ship_.resource = res;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_max_resource(resource_t max_res) {
  ship_.max_resource = max_res;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_destruct(resource_t destruct) {
  ship_.destruct = destruct;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_max_destruct(resource_t max_destruct) {
  ship_.max_destruct = max_destruct;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_crystals(crystal_t crystals) {
  ship_.crystals = crystals;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_mount(unsigned char mount) {
  ship_.mount = mount;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_damage(damage_t damage) {
  ship_.damage = damage;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_radiation(radiation_t rad) {
  ship_.rad = rad;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_armor(armor_t armor) {
  ship_.armor = armor;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_mass(double mass) {
  ship_.mass = mass;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_size(ship_size_t size) {
  ship_.size = size;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_on(bool on) {
  ship_.on = on;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_special(SpecialData special) {
  ship_.special = special;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_trigger_radius(unsigned short radius) {
  ship_.special = TriggerData{.radius = radius};
  return *this;
}

TestShipBuilder& TestShipBuilder::targeting_planet(starnum_t snum,
                                                   planetnum_t pnum) {
  ship_.whatdest = ScopeLevel::LEVEL_PLAN;
  ship_.deststar = snum;
  ship_.destpnum = pnum;
  return *this;
}

TestShipBuilder& TestShipBuilder::targeting_ship(shipnum_t target_ship) {
  ship_.whatdest = ScopeLevel::LEVEL_SHIP;
  ship_.destshipno = target_ship;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_impact(Coordinates coords,
                                              bool scatter) {
  ship_.special = ImpactData{.coords = coords, .scatter = scatter};
  return *this;
}

TestShipBuilder& TestShipBuilder::with_aim(AimedAtData aim) {
  ship_.special = aim;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_pod(unsigned char temp,
                                           unsigned char decay) {
  ship_.special = PodData{.decay = decay, .temperature = temp};
  return *this;
}

EntityHandle<Ship> TestShipBuilder::build_handle() {
  auto ship_obj = ShipFactory::create(std::move(ship_));
  return em_.create_ship(std::move(ship_obj));
}

shipnum_t TestShipBuilder::build() {
  return build_handle()->number();
}

TestWorldBuilder::TestWorldBuilder(TestContext& ctx) : store_(ctx.db) {}
TestWorldBuilder::TestWorldBuilder(Database& db) : store_(db) {}

TestWorldBuilder&
TestWorldBuilder::add_race(std::string_view name, double tech, bool guest,
                           std::optional<player_t> explicit_id) {
  player_t id = explicit_id.value_or(
      player_t{static_cast<player_t::value_type>(next_player_id_++)});
  Race race{};
  race.Playernum = id;
  race.name = name;
  race.tech = tech;
  race.Guest = guest;
  race.governor[0].active = true;
  race.governor[0].money = 10'000;
  race.mass = 1.0;
  race.metabolism = 1.0;
  RaceRepository(store_).save(race);
  registered_races_.push_back(id);
  return *this;
}

TestWorldBuilder&
TestWorldBuilder::add_star(std::string_view name, ap_t initial_ap,
                           std::optional<starnum_t> explicit_snum) {
  starnum_t snum = explicit_snum.value_or(
      starnum_t{static_cast<starnum_t::value_type>(next_star_id_++)});
  star_struct ss{};
  ss.star_id = snum;
  ss.name = name;
  ss.AP.fill(initial_ap);
  Star star{ss};
  for (player_t pid : registered_races_) {
    star.mark_explored_by(pid);
    star.mark_inhabited_by(pid);
  }
  StarRepository(store_).save(star);
  registered_stars_.push_back(snum);

  UniverseRepository univ_repo(store_);
  auto u = univ_repo.find(1);
  if (u) {
    if (snum.value + 1 > u->numstars) {
      u->numstars = snum.value + 1;
      univ_repo.save(*u);
    }
  }
  return *this;
}

TestWorldBuilder& TestWorldBuilder::add_planet(
    starnum_t snum, PlanetType type, std::string_view name, unsigned char maxx,
    unsigned char maxy, std::optional<planetnum_t> explicit_pnum) {
  planetnum_t pnum{0};
  if (explicit_pnum) {
    pnum = *explicit_pnum;
  } else {
    StarRepository stars(store_);
    auto star_opt = stars.find(snum);
    pnum = planetnum_t{static_cast<planetnum_t::value_type>(
        star_opt ? star_opt->numplanets() : 0)};
  }
  Planet p(type, Coordinates{maxx, maxy});
  p.star_id() = snum;
  p.planet_order() = pnum;
  p.explored() = true;
  for (player_t pid : registered_races_) {
    p.info(pid).explored = 1;
    p.info(pid).destruct = 1000;
    p.info(pid).fuel = 1000;
    p.info(pid).resource = 1000;
  }
  PlanetRepository(store_).save(p);

  // Keep star planet names synchronized
  StarRepository stars(store_);
  auto star_opt = stars.find(snum);
  if (star_opt) {
    std::string planet_name =
        name.empty() ? std::format("Planet-{}", pnum.value) : std::string(name);
    star_opt->set_planet_name(pnum, planet_name);
    stars.save(*star_opt);
  }

  // Save initial SectorMap with coordinate indexing
  SectorMap smap(p);
  for (int y = 0; y < maxy; ++y) {
    for (int x = 0; x < maxx; ++x) {
      smap.get(Coordinates{Coordinates{x, y}}).set_x(x);
      smap.get(Coordinates{Coordinates{x, y}}).set_y(y);
    }
  }
  SectorRepository(store_).save_map(smap);
  return *this;
}

void TestWorldBuilder::create_standard_solar_system(TestContext& ctx) {
  ctx.with_standard_universe();
}

TestPlanetBuilder::TestPlanetBuilder(EntityManager& em, Database& db,
                                     starnum_t snum, PlanetType type,
                                     Coordinates dims,
                                     std::optional<planetnum_t> explicit_pnum)
    : em_(em), db_(db), snum_(snum), explicit_pnum_(explicit_pnum),
      planet_([&]() {
        Planet p{type, dims};
        p.star_id() = snum;
        if (explicit_pnum) {
          p.planet_order() = *explicit_pnum;
        } else {
          try {
            const auto* star = em.peek_star(snum);
            p.planet_order() = planetnum_t{
                static_cast<planetnum_t::value_type>(star->numplanets())};
          } catch (const EntityNotFoundError&) {
            p.planet_order() = planetnum_t{0};
          }
        }
        return p;
      }()),
      smap_(planet_) {
  for (int y = 0; y < dims.y; ++y) {
    for (int x = 0; x < dims.x; ++x) {
      smap_.get(Coordinates{x, y}).set_x(x);
      smap_.get(Coordinates{x, y}).set_y(y);
    }
  }
}

TestPlanetBuilder::TestPlanetBuilder(TestContext& ctx, starnum_t snum,
                                     PlanetType type, Coordinates dims,
                                     std::optional<planetnum_t> explicit_pnum)
    : TestPlanetBuilder(ctx.em, ctx.db, snum, type, dims, explicit_pnum) {}

TestPlanetBuilder& TestPlanetBuilder::named(std::string_view name) {
  name_ = name;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_type(PlanetType type) {
  planet_.type() = type;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_dimensions(Coordinates dims) {
  planet_.dimensions() = dims;
  smap_ = SectorMap(planet_);
  for (int y = 0; y < dims.y; ++y) {
    for (int x = 0; x < dims.x; ++x) {
      smap_.get(Coordinates{x, y}).set_x(x);
      smap_.get(Coordinates{x, y}).set_y(y);
    }
  }
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_position(SystemCoordinates coords) {
  planet_.set_system_coordinates(coords);
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_toxicity(int toxic) {
  planet_.conditions(TOXIC) = toxic;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_temperature(int temp) {
  planet_.conditions(TEMP) = temp;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_explored(player_t player,
                                                    bool explored) {
  planet_.info(player).explored = explored ? 1 : 0;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_stockpiles(player_t player,
                                                      resource_t res,
                                                      resource_t fuel,
                                                      resource_t destruct) {
  planet_.info(player).resource = res;
  planet_.info(player).fuel = fuel;
  planet_.info(player).destruct = destruct;
  return *this;
}

TestPlanetBuilder&
TestPlanetBuilder::with_sector(Coordinates coords, SectorType type, int fert,
                               int eff, resource_t res, player_t owner,
                               population_t popn, population_t troops) {
  auto& sect = smap_.get(coords);
  sect.set_condition(type);
  sect.set_fert(fert);
  sect.set_efficiency_bounded(eff);
  sect.set_resource(res);
  if (owner.value > 0 && (popn > 0 || troops > 0)) {
    sect.colonize(owner, popn);
    if (troops > 0) {
      sect.set_troops(troops);
    }
  }
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_all_sectors(SectorType type,
                                                       int fert, int eff,
                                                       resource_t res) {
  for (auto& sect : smap_) {
    sect.set_condition(type);
    sect.set_fert(fert);
    sect.set_efficiency_bounded(eff);
    sect.set_resource(res);
  }
  return *this;
}

TestPlanetBuilder&
TestPlanetBuilder::with_colony(player_t owner, population_t popn,
                               Coordinates capital_coords, int fert, int eff,
                               resource_t res, population_t troops) {
  with_sector(capital_coords, SectorType::SEC_LAND, fert, eff, res, owner, popn,
              troops);
  with_explored(owner, true);
  return *this;
}

planetnum_t TestPlanetBuilder::build() {
  planetnum_t pnum{0};
  if (explicit_pnum_) {
    pnum = *explicit_pnum_;
  } else {
    try {
      const auto* star = em_.peek_star(snum_);
      pnum =
          planetnum_t{static_cast<planetnum_t::value_type>(star->numplanets())};
    } catch (const EntityNotFoundError&) {
      pnum = planetnum_t{0};
    }
  }
  planet_.planet_order() = pnum;
  if (pnum != smap_.planet_order()) {
    SectorMap updated_smap(planet_);
    for (Sector& sect : smap_) {
      updated_smap.get(Coordinates{static_cast<int>(sect.get_x()),
                                   static_cast<int>(sect.get_y())}) =
          std::move(sect);
    }
    smap_ = std::move(updated_smap);
  }

  // Calculate sector-aggregate invariants
  population_t total_pop = 0;
  population_t total_troops = 0;
  PlayerVector<population_t, MAXPLAYERS> player_pop{};
  PlayerVector<population_t, MAXPLAYERS> player_troops{};
  PlayerVector<int, MAXPLAYERS> player_sects{};

  for (const auto& sect : smap_) {
    total_pop += sect.get_popn();
    total_troops += sect.get_troops();
    if (sect.get_owner().value > 0) {
      player_pop[sect.get_owner()] += sect.get_popn();
      player_troops[sect.get_owner()] += sect.get_troops();
      if (sect.is_populated() || sect.is_owned()) {
        player_sects[sect.get_owner()]++;
      }
    }
  }

  planet_.popn() = total_pop;
  planet_.troops() = total_troops;
  planet_.maxpopn() = std::max(total_pop, population_t{10000});
  for (player_t p = 1; p <= MAXPLAYERS; ++p) {
    if (player_sects[p] > 0 || player_pop[p] > 0) {
      planet_.info(p).popn = player_pop[p];
      planet_.info(p).troops = player_troops[p];
      planet_.info(p).numsectsowned = player_sects[p];
    }
  }

  JsonStore store(db_);
  PlanetRepository(store).save(planet_);
  SectorRepository(store).save_map(smap_);

  try {
    em_.mutate_star(snum_, [&](Star& s) {
      std::string planet_name =
          name_.empty() ? std::format("Planet-{}", pnum.value) : name_;
      s.set_planet_name(pnum, planet_name);
    });
  } catch (const EntityNotFoundError&) {
    // Star not present in EntityManager
  }

  em_.clear_cache();
  return pnum;
}

const Planet* TestPlanetBuilder::build_and_peek() {
  planetnum_t pnum = build();
  return em_.peek_planet(snum_, pnum);
}
