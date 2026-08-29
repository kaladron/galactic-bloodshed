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

TestShipBuilder::TestShipBuilder(EntityManager& em, ShipType type,
                                 std::optional<shipnum_t> explicit_number)
    : em_(em) {
  shipnum_t number = explicit_number.value_or(
      shipnum_t{static_cast<shipnum_t::value_type>(em.num_ships().value + 1)});
  ship_.number = number;
  ship_.type = type;
  ship_.build_type = type;
  ship_.alive = true;
  ship_.active = true;
  ship_.on = true;
  ship_.owner = 1;
  ship_.governor = 0;
  ship_.tech = 100.0;
  ship_.name = Shipnames[type];

  // Canonical baseline initialization from Shipdata
  ship_.armor = static_cast<unsigned char>(Shipdata[type][ABIL_ARMOR]);
  ship_.max_crew = static_cast<unsigned short>(Shipdata[type][ABIL_MAXCREW]);
  ship_.max_resource = static_cast<resource_t>(Shipdata[type][ABIL_CARGO]);
  ship_.max_destruct =
      static_cast<unsigned short>(Shipdata[type][ABIL_DESTCAP]);
  ship_.max_fuel = static_cast<unsigned short>(Shipdata[type][ABIL_FUELCAP]);
  ship_.max_speed = static_cast<unsigned short>(Shipdata[type][ABIL_SPEED]);
  ship_.build_cost = static_cast<unsigned short>(Shipdata[type][ABIL_COST]);
  ship_.fuel = static_cast<double>(ship_.max_fuel);
  ship_.destruct = static_cast<unsigned short>(ship_.max_destruct);
  ship_.hanger = 0;
  ship_.max_hanger = static_cast<hangar_t>(Shipdata[type][ABIL_HANGER]);
  ship_.primtype = static_cast<guntype_t>(Shipdata[type][ABIL_PRIMARY]);
  ship_.sectype = static_cast<guntype_t>(Shipdata[type][ABIL_SECONDARY]);
  ship_.guns = static_cast<gun_count_t>(
      Shipdata[type][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE);
  ship_.primary = static_cast<weapon_power_t>(Shipdata[type][ABIL_GUNS]);
  ship_.retaliate = ship_.primary;

  // Calculate baseline size and mass using canonical ship functions
  Ship temp_ship{ship_};
  ship_.size = static_cast<ship_size_t>(ship_size(temp_ship));
  ship_.base_mass = getmass(temp_ship);
  ship_.mass = ship_.base_mass;
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

TestShipBuilder& TestShipBuilder::in_star_orbit(starnum_t snum, double x,
                                                double y) {
  ship_.whatorbits = ScopeLevel::LEVEL_STAR;
  ship_.storbits = snum;
  ship_.pnumorbits = 0;
  ship_.xpos = x;
  ship_.ypos = y;
  ship_.docked = 0;
  return *this;
}

TestShipBuilder& TestShipBuilder::in_planet_orbit(starnum_t snum,
                                                  planetnum_t pnum, double x,
                                                  double y) {
  ship_.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship_.storbits = snum;
  ship_.pnumorbits = pnum;
  ship_.xpos = x;
  ship_.ypos = y;
  ship_.docked = 0;
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
  return *this;
}

TestShipBuilder& TestShipBuilder::docked_to(shipnum_t dest_ship,
                                            starnum_t snum) {
  ship_.whatorbits = ScopeLevel::LEVEL_SHIP;
  ship_.destshipno = dest_ship;
  ship_.storbits = snum;
  ship_.docked = 1;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_guns(guntype_t primtype,
                                            unsigned long count,
                                            unsigned char guns_flag) {
  ship_.guns = guns_flag;
  ship_.primtype = primtype;
  ship_.primary = static_cast<weapon_power_t>(count);
  ship_.retaliate = static_cast<weapon_power_t>(count);
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

TestShipBuilder& TestShipBuilder::with_fuel(double fuel) {
  ship_.fuel = fuel;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_resource(resource_t res) {
  ship_.resource = res;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_destruct(unsigned short destruct) {
  ship_.destruct = destruct;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_damage(damage_t damage) {
  ship_.damage = damage;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_armor(armor_t armor) {
  ship_.armor = armor;
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
  TestWorldBuilder(ctx)
      .add_race("Federation", 100.0)
      .add_race("Klingons", 100.0)
      .add_star("Sol", 100)
      .add_planet(0, PlanetType::EARTH, "Earth");
}
