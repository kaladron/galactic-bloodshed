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
  ship_.storbits = std::nullopt;
  ship_.pnumorbits = std::nullopt;
  ship_.deststar = std::nullopt;
  ship_.destpnum = std::nullopt;

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
  ship_.pnumorbits = std::nullopt;
  ship_.dock_state = DockState::Spaceborne;
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
  ship_.pnumorbits = std::nullopt;
  ship_.dock_state = DockState::Spaceborne;
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
  ship_.dock_state = DockState::Spaceborne;
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
  ship_.dock_state = DockState::Spaceborne;
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
  ship_.deststar = snum;
  ship_.destpnum = pnum;
  ship_.dock_state = DockState::Landed;
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

TestShipBuilder& TestShipBuilder::in_deep_space(UniverseCoordinates coords) {
  ship_.whatorbits = ScopeLevel::LEVEL_UNIV;
  ship_.storbits = std::nullopt;
  ship_.pnumorbits = std::nullopt;
  ship_.dock_state = DockState::Spaceborne;
  ship_.coordinates = coords;
  return *this;
}

TestShipBuilder& TestShipBuilder::docked_to(shipnum_t dest_ship,
                                            starnum_t snum) {
  ship_.whatorbits = ScopeLevel::LEVEL_SHIP;
  ship_.whatdest = ScopeLevel::LEVEL_SHIP;
  ship_.destshipno = dest_ship;
  ship_.storbits = snum;
  ship_.pnumorbits = std::nullopt;
  ship_.deststar = std::nullopt;
  ship_.destpnum = std::nullopt;
  ship_.dock_state = DockState::Docked;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_build_type(ShipType build_type) {
  ship_.build_type = build_type;
  if (ship_.type == ShipType::OTYPE_FACTORY &&
      build_type != ShipType::OTYPE_FACTORY) {
    Ship temp{ship_};
    temp.set_factory_blueprint(build_type);
    ship_ = temp.to_struct();
  }
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
                                           weapon_range_t range) {
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

TestShipBuilder& TestShipBuilder::with_hanger(hangar_t hanger) {
  ship_.hanger = hanger;
  return *this;
}

TestShipBuilder& TestShipBuilder::with_max_hanger(hangar_t max_hanger) {
  ship_.max_hanger = max_hanger;
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

TestShipBuilder& TestShipBuilder::with_trigger_radius(weapon_range_t radius) {
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
  ship_.deststar = std::nullopt;
  ship_.destpnum = std::nullopt;
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
  race.leader().money = 10'000;
  race.mass = 1.0;
  race.metabolism = 1.0;
  RaceRepository(store_).save(race);
  BlockRepository blocks(store_);
  if (!blocks.find_by_id(id)) {
    block b{};
    b.Playernum = id;
    b.name = std::string(name);
    blocks.save(b);
  }
  PowerRepository powers(store_);
  if (!powers.find_by_id(id)) {
    power p{};
    p.id = id;
    powers.save(p);
  }
  registered_races_.push_back(id);
  return *this;
}

TestWorldBuilder&
TestWorldBuilder::add_star(std::string_view name, ap_t initial_ap,
                           std::optional<starnum_t> explicit_snum) {
  starnum_t snum = explicit_snum.value_or(
      starnum_t{static_cast<starnum_t::value_type>(next_star_id_++)});
  Star star{snum, name};
  for (player_t pid : registered_races_) {
    star.AP(pid) = initial_ap;
    star.mark_explored_by(pid);
    star.mark_inhabited_by(pid);
  }
  StarRepository(store_).save(star);
  registered_stars_.push_back(snum);

  return *this;
}

TestWorldBuilder& TestWorldBuilder::add_planet(
    starnum_t snum, PlanetType type, std::string_view name, unsigned char maxx,
    unsigned char maxy, std::optional<planetnum_t> explicit_pnum) {
  planetnum_t pnum{1};
  if (explicit_pnum) {
    pnum = *explicit_pnum;
  } else {
    StarRepository stars(store_);
    auto star_opt = stars.find(snum);
    pnum = planetnum_t{static_cast<planetnum_t::value_type>(
        star_opt ? star_opt->numplanets() + 1 : 1)};
  }
  Planet p{snum, pnum, type, Coordinates{maxx, maxy}};
  p.explored() = true;
  for (player_t pid : registered_races_) {
    p.info(pid).explored = 1;
    p.info(pid).destruct = 1000;
    p.info(pid).fuel = 1000;
    p.info(pid).resource = 1000;
  }

  // Keep star planet names synchronized before saving child planet
  StarRepository stars(store_);
  auto star_opt = stars.find(snum);
  if (star_opt) {
    std::string planet_name =
        name.empty() ? std::format("Planet-{}", pnum.value) : std::string(name);
    star_opt->set_planet_name(pnum, planet_name);
    stars.save(*star_opt);
  }

  PlanetRepository(store_).save(p);

  // Save initial SectorMap with coordinate indexing
  SectorMap smap(p);
  for (int y = 0; y < maxy; ++y) {
    for (int x = 0; x < maxx; ++x) {
      smap.get(Coordinates{x, y}).set_coords({x, y});
    }
  }
  SectorRepository(store_).save_map(smap);
  return *this;
}

void TestWorldBuilder::create_standard_solar_system(TestContext& ctx) {
  ctx.with_standard_universe();
}

TestStarBuilder::TestStarBuilder(EntityManager& em, Database& db,
                                 std::string_view name,
                                 std::optional<starnum_t> explicit_snum)
    : em_(em), db_(db), star_([&]() {
        starnum_t snum = explicit_snum.value_or([&]() {
          JsonStore store(db);
          return starnum_t{static_cast<starnum_t::value_type>(
              store.find_next_available_id("tbl_star"))};
        }());
        return Star{snum, name};
      }()) {}

TestStarBuilder::TestStarBuilder(TestContext& ctx, std::string_view name,
                                 std::optional<starnum_t> explicit_snum)
    : TestStarBuilder(ctx.em, ctx.db, name, explicit_snum) {}

TestStarBuilder& TestStarBuilder::named(std::string_view name) {
  star_.set_name(name);
  return *this;
}

TestStarBuilder& TestStarBuilder::with_position(UniverseCoordinates coords) {
  star_.set_coordinates(coords);
  return *this;
}

TestStarBuilder& TestStarBuilder::with_position(double x, double y) {
  star_.set_coordinates(UniverseCoordinates{x, y});
  return *this;
}

TestStarBuilder& TestStarBuilder::with_stability(int stability) {
  star_.stability() = stability;
  return *this;
}

TestStarBuilder& TestStarBuilder::with_nova_stage(int stage) {
  star_.nova_stage() = stage;
  return *this;
}

TestStarBuilder& TestStarBuilder::with_temperature(int temp) {
  star_.temperature() = temp;
  return *this;
}

TestStarBuilder& TestStarBuilder::with_gravity(double grav) {
  star_.gravity() = grav;
  return *this;
}

TestStarBuilder& TestStarBuilder::with_ap(player_t player, ap_t ap) {
  star_.AP(player) = ap;
  return *this;
}

TestStarBuilder& TestStarBuilder::with_governor(player_t player,
                                                governor_t gov) {
  star_.set_governor(player, gov);
  return *this;
}

TestStarBuilder& TestStarBuilder::with_explored(player_t player,
                                                bool explored) {
  if (explored) {
    star_.mark_explored_by(player);
  } else {
    star_.clear_explored_by(player);
  }
  return *this;
}

TestStarBuilder& TestStarBuilder::with_inhabited(player_t player,
                                                 bool inhabited) {
  if (inhabited) {
    star_.mark_inhabited_by(player);
  } else {
    star_.clear_inhabited_by(player);
  }
  return *this;
}

TestStarBuilder& TestStarBuilder::with_planet_name(planetnum_t pnum,
                                                   std::string_view name) {
  star_.set_planet_name(pnum, name);
  return *this;
}

TestStarBuilder& TestStarBuilder::with_planet_names(
    std::initializer_list<std::string_view> names) {
  planetnum_t pnum{1};
  for (std::string_view name : names) {
    star_.set_planet_name(pnum, name);
    ++pnum;
  }
  return *this;
}

starnum_t TestStarBuilder::build() {
  JsonStore store(db_);
  StarRepository(store).save(star_);
  em_.clear_cache();
  return star_.star_id();
}

const Star* TestStarBuilder::build_and_peek() {
  starnum_t snum = build();
  return em_.peek_star(snum);
}

TestPlanetBuilder::TestPlanetBuilder(EntityManager& em, Database& db,
                                     starnum_t snum, PlanetType type,
                                     Coordinates dims,
                                     std::optional<planetnum_t> explicit_pnum)
    : em_(em), db_(db), snum_(snum), explicit_pnum_(explicit_pnum),
      planet_([&]() {
        planetnum_t pnum{1};
        if (explicit_pnum) {
          pnum = *explicit_pnum;
        } else {
          try {
            const auto* star = em.peek_star(snum);
            pnum = planetnum_t{
                static_cast<planetnum_t::value_type>(star->numplanets() + 1)};
          } catch (const EntityNotFoundError&) {
            pnum = planetnum_t{1};
          }
        }
        return Planet{snum, pnum, type, dims};
      }()),
      smap_(planet_) {
  for (int y = 0; y < dims.y; ++y) {
    for (int x = 0; x < dims.x; ++x) {
      smap_.get(Coordinates{x, y}).set_coords({x, y});
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
      smap_.get(Coordinates{x, y}).set_coords({x, y});
    }
  }
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_position(SystemCoordinates coords) {
  planet_.set_system_coordinates(coords);
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_toxicity(int toxic) {
  planet_.toxic() = toxic;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_temperature(int temp) {
  planet_.temp() = temp;
  planet_.rtemp() = temp;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_rtemp(int rtemp) {
  planet_.rtemp() = rtemp;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_condition(AtmosphereConditions cond,
                                                     int pct) {
  planet_.conditions(cond) = pct;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_explored(bool explored) {
  planet_.explored() = explored ? 1 : 0;
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_explored(player_t player,
                                                    bool explored) {
  planet_.info(player).explored = explored ? 1 : 0;
  if (explored) {
    planet_.explored() = 1;
  }
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_enslaved_to(player_t master) {
  planet_.enslave_to(master);
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_tax(player_t player, int tax,
                                               std::optional<int> newtax) {
  planet_.info(player).tax = tax;
  planet_.info(player).newtax = newtax.value_or(tax);
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_crystals(player_t player,
                                                    crystal_t crystals) {
  planet_.info(player).crystals = crystals;
  return *this;
}

TestPlanetBuilder&
TestPlanetBuilder::with_route(player_t player, int route_index,
                              starnum_t dest_star, planetnum_t dest_planet,
                              CommodityManifest load, CommodityManifest unload,
                              Coordinates coords) {
  auto& r =
      planet_.info(player).route.at(static_cast<std::size_t>(route_index));
  r.set = true;
  r.set_destination(dest_star, dest_planet);
  r.load = load;
  r.unload = unload;
  r.dest_coords = coords;
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
  sect.set_type(type);
  sect.set_fert(fert);
  sect.set_efficiency_bounded(eff);
  sect.set_resource(res);
  if (owner.value > 0) {
    if (popn > 0) {
      sect.colonize(owner, popn);
    } else {
      sect.set_owner(owner);
    }
    if (troops > 0) {
      sect.set_troops(troops);
    }
  }
  return *this;
}

TestPlanetBuilder& TestPlanetBuilder::with_all_sectors(SectorType type,
                                                       int fert, int eff,
                                                       resource_t res,
                                                       player_t owner) {
  for (auto& sect : smap_) {
    sect.set_condition(type);
    sect.set_type(type);
    sect.set_fert(fert);
    sect.set_efficiency_bounded(eff);
    sect.set_resource(res);
    if (owner.value > 0) {
      sect.set_owner(owner);
    }
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
  const planetnum_t pnum = planet_.planet_order();

  // Synchronize sector-aggregate demographic invariants
  planet_.sync_demographics(smap_);
  planet_.maxpopn() = std::max(planet_.popn(), population_t{10000});

  try {
    em_.mutate_star(snum_, [&](Star& s) {
      std::string planet_name =
          name_.empty() ? std::format("Planet-{}", pnum.value) : name_;
      s.set_planet_name(pnum, planet_name);
    });
  } catch (const EntityNotFoundError&) {
    // Star not present in EntityManager
  }

  JsonStore store(db_);
  PlanetRepository(store).save(planet_);
  SectorRepository(store).save_map(smap_);

  em_.clear_cache();
  return pnum;
}

const Planet* TestPlanetBuilder::build_and_peek() {
  planetnum_t pnum = build();
  return em_.peek_planet(snum_, pnum);
}
