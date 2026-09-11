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
    const auto* star = em_.peek_star(snum);
    ship_.coordinates = star->coordinates();
  }
  return *this;
}

TestShipBuilder& TestShipBuilder::in_star_orbit(starnum_t snum,
                                                SystemCoordinates coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_STAR;
  ship_.storbits = snum;
  ship_.docked = 0;
  const auto* star = em_.peek_star(snum);
  ship_.coordinates = star->coordinates() + coords;
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
    const auto* star = em_.peek_star(snum);
    const auto* planet = em_.peek_planet(snum, pnum);
    ship_.coordinates = planet->absolute_coordinates(*star);
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
  const auto* star = em_.peek_star(snum);
  ship_.coordinates = star->coordinates() + coords;
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
  const auto* star = em_.peek_star(snum);
  const auto* planet = em_.peek_planet(snum, pnum);
  ship_.coordinates = planet->absolute_coordinates(*star);
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

TestShipBuilder& TestShipBuilder::with_nextship(shipnum_t next) {
  ship_.nextship = next;
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

shipnum_t TestShipBuilder::build() {
  auto handle = em_.create_ship(ship_);
  return handle->number();
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
