// SPDX-License-Identifier: Apache-2.0

module;

#include <glaze/glaze.hpp>

export module gblib:repositories;

import dallib;
import :types;
import :race;
import :ships;
import :star;
import :planet;
import :sector;
import std.compat;

// Base template for repositories
// Provides common CRUD operations for entity types
// Derived classes must implement serialize/deserialize for their specific type
export template <typename T>
class Repository {
 protected:
  JsonStore& store;
  std::string table_name;

  // Derived classes must implement these for their specific type
  virtual std::optional<std::string> serialize(const T& entity) const = 0;
  virtual std::optional<T> deserialize(const std::string& json) const = 0;

 public:
  Repository(JsonStore& js, std::string table)
      : store(js), table_name(std::move(table)) {}

  virtual ~Repository() = default;

  // Delete copy, allow move
  Repository(const Repository&) = delete;
  Repository& operator=(const Repository&) = delete;
  Repository(Repository&&) = default;
  Repository& operator=(Repository&&) = default;

  // Save entity with given ID
  bool save(int id, const T& entity) {
    auto json = serialize(entity);
    if (!json) return false;
    return store.store(table_name, id, *json);
  }

  // Find entity by ID
  std::optional<T> find(int id) {
    auto json = store.retrieve(table_name, id);
    if (!json) return std::nullopt;
    return deserialize(*json);
  }

  // Remove entity by ID
  bool remove(int id) { return store.remove(table_name, id); }

  // Get next available ID
  int next_available_id() { return store.find_next_available_id(table_name); }

  // List all IDs in the table
  std::vector<int> list_ids() { return store.list_ids(table_name); }
};

// Glaze reflection for Race (must be in global glz namespace)
namespace glz {
template <>
struct meta<toggletype> {
  using T = toggletype;
  static constexpr auto value = object(
      "invisible", &T::invisible, "standby", &T::standby, "color", &T::color,
      "gag", &T::gag, "double_digits", &T::double_digits, "inverse",
      &T::inverse, "geography", &T::geography, "autoload", &T::autoload,
      "highlight", &T::highlight, "compat", &T::compat);
};

template <>
struct meta<Race::gov> {
  using T = Race::gov;
  static constexpr auto value =
      object("name", &T::name, "password", &T::password, "active", &T::active,
             "deflevel", &T::deflevel, "defsystem", &T::defsystem,
             "defplanetnum", &T::defplanetnum, "homelevel", &T::homelevel,
             "homesystem", &T::homesystem, "homeplanetnum", &T::homeplanetnum,
             "newspos", &T::newspos, "toggle", &T::toggle, "money", &T::money,
             "income", &T::income, "maintain", &T::maintain, "cost_tech",
             &T::cost_tech, "cost_market", &T::cost_market, "profit_market",
             &T::profit_market, "login", &T::login);
};

template <>
struct meta<Race> {
  using T = Race;
  static constexpr auto value = object(
      "Playernum", &T::Playernum, "name", &T::name, "password", &T::password,
      "info", &T::info, "motto", &T::motto, "absorb", &T::absorb,
      "collective_iq", &T::collective_iq, "pods", &T::pods, "fighters",
      &T::fighters, "IQ", &T::IQ, "IQ_limit", &T::IQ_limit, "number_sexes",
      &T::number_sexes, "fertilize", &T::fertilize, "adventurism",
      &T::adventurism, "birthrate", &T::birthrate, "mass", &T::mass,
      "metabolism", &T::metabolism, "conditions", &T::conditions, "likes",
      &T::likes, "likesbest", &T::likesbest, "dissolved", &T::dissolved, "God",
      &T::God, "Guest", &T::Guest, "Metamorph", &T::Metamorph, "monitor",
      &T::monitor, "translate", &T::translate, "atwar", &T::atwar, "allied",
      &T::allied, "Gov_ship", &T::Gov_ship, "morale", &T::morale, "points",
      &T::points, "controlled_planets", &T::controlled_planets, "victory_turns",
      &T::victory_turns, "turn", &T::turn, "tech", &T::tech, "discoveries",
      &T::discoveries, "victory_score", &T::victory_score, "votes", &T::votes,
      "planet_points", &T::planet_points, "governors", &T::governors,
      "governor", &T::governor);
};
}  // namespace glz

// RaceRepository - provides type-safe access to Race entities
export class RaceRepository : public Repository<Race> {
 public:
  RaceRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Race> find_by_player(player_t player);
  bool save_race(const Race& race);

 protected:
  std::optional<std::string> serialize(const Race& race) const override;
  std::optional<Race> deserialize(const std::string& json_str) const override;
};

// RaceRepository implementation
RaceRepository::RaceRepository(JsonStore& store)
    : Repository<Race>(store, "tbl_race") {}

std::optional<std::string> RaceRepository::serialize(const Race& race) const {
  auto result = glz::write_json(race);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Race> RaceRepository::deserialize(
    const std::string& json_str) const {
  Race race{};
  auto result = glz::read_json(race, json_str);
  if (!result) {
    return race;
  }
  return std::nullopt;
}

std::optional<Race> RaceRepository::find_by_player(player_t player) {
  return find(player);
}

bool RaceRepository::save_race(const Race& race) {
  return save(race.Playernum, race);
}

// Glaze reflection for Ship special function data structures
namespace glz {
template <>
struct meta<AimedAtData> {
  using T = AimedAtData;
  static constexpr auto value =
      object("shipno", &T::shipno, "snum", &T::snum, "intensity", &T::intensity,
             "pnum", &T::pnum, "level", &T::level);
};

template <>
struct meta<MindData> {
  using T = MindData;
  static constexpr auto value =
      object("progenitor", &T::progenitor, "target", &T::target, "generation",
             &T::generation, "busy", &T::busy, "tampered", &T::tampered,
             "who_killed", &T::who_killed);
};

template <>
struct meta<PodData> {
  using T = PodData;
  static constexpr auto value =
      object("decay", &T::decay, "temperature", &T::temperature);
};

template <>
struct meta<TimerData> {
  using T = TimerData;
  static constexpr auto value = object("count", &T::count);
};

template <>
struct meta<ImpactData> {
  using T = ImpactData;
  static constexpr auto value =
      object("x", &T::x, "y", &T::y, "scatter", &T::scatter);
};

template <>
struct meta<TriggerData> {
  using T = TriggerData;
  static constexpr auto value = object("radius", &T::radius);
};

template <>
struct meta<TerraformData> {
  using T = TerraformData;
  static constexpr auto value = object("index", &T::index);
};

template <>
struct meta<TransportData> {
  using T = TransportData;
  static constexpr auto value = object("target", &T::target);
};

template <>
struct meta<WasteData> {
  using T = WasteData;
  static constexpr auto value = object("toxic", &T::toxic);
};

// Glaze reflection for anonymous structs in Ship class
template <>
struct meta<decltype(Ship::navigate)> {
  using T = decltype(Ship::navigate);
  static constexpr auto value =
      object("on", &T::on, "speed", &T::speed, "turns", &T::turns, "bearing",
             &T::bearing);
};

template <>
struct meta<decltype(Ship::protect)> {
  using T = decltype(Ship::protect);
  static constexpr auto value =
      object("maxrng", &T::maxrng, "on", &T::on, "planet", &T::planet, "self",
             &T::self, "evade", &T::evade, "ship", &T::ship);
};

template <>
struct meta<decltype(Ship::hyper_drive)> {
  using T = decltype(Ship::hyper_drive);
  static constexpr auto value = object("charge", &T::charge, "ready", &T::ready,
                                       "on", &T::on, "has", &T::has);
};

// Glaze reflection for Ship class
template <>
struct meta<Ship> {
  using T = Ship;
  static constexpr auto value = object(
      "number", &T::number, "owner", &T::owner, "governor", &T::governor,
      "name", &T::name, "shipclass", &T::shipclass, "race", &T::race, "xpos",
      &T::xpos, "ypos", &T::ypos, "fuel", &T::fuel, "mass", &T::mass, "land_x",
      &T::land_x, "land_y", &T::land_y, "destshipno", &T::destshipno,
      "nextship", &T::nextship, "ships", &T::ships, "armor", &T::armor, "size",
      &T::size, "max_crew", &T::max_crew, "max_resource", &T::max_resource,
      "max_destruct", &T::max_destruct, "max_fuel", &T::max_fuel, "max_speed",
      &T::max_speed, "build_type", &T::build_type, "build_cost", &T::build_cost,
      "base_mass", &T::base_mass, "tech", &T::tech, "complexity",
      &T::complexity, "destruct", &T::destruct, "resource", &T::resource,
      "popn", &T::popn, "troops", &T::troops, "crystals", &T::crystals,
      "special", &T::special, "who_killed", &T::who_killed, "navigate",
      &T::navigate, "protect", &T::protect, "mount", &T::mount, "hyper_drive",
      &T::hyper_drive, "cew", &T::cew, "cew_range", &T::cew_range, "cloak",
      &T::cloak, "laser", &T::laser, "focus", &T::focus, "fire_laser",
      &T::fire_laser, "storbits", &T::storbits, "deststar", &T::deststar,
      "destpnum", &T::destpnum, "pnumorbits", &T::pnumorbits, "whatdest",
      &T::whatdest, "whatorbits", &T::whatorbits, "damage", &T::damage, "rad",
      &T::rad, "retaliate", &T::retaliate, "target", &T::target, "type",
      &T::type, "speed", &T::speed, "active", &T::active, "alive", &T::alive,
      "mode", &T::mode, "bombard", &T::bombard, "mounted", &T::mounted,
      "cloaked", &T::cloaked, "sheep", &T::sheep, "docked", &T::docked,
      "notified", &T::notified, "examined", &T::examined, "on", &T::on,
      "merchant", &T::merchant, "guns", &T::guns, "primary", &T::primary,
      "primtype", &T::primtype, "secondary", &T::secondary, "sectype",
      &T::sectype, "hanger", &T::hanger, "max_hanger", &T::max_hanger);
};
}  // namespace glz

// ShipRepository - provides type-safe access to Ship entities
export class ShipRepository : public Repository<Ship> {
 public:
  ShipRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Ship> find_by_number(shipnum_t num);
  bool save_ship(const Ship& ship);
  void delete_ship(shipnum_t num);
  shipnum_t next_ship_number();
  shipnum_t count_all_ships();

 protected:
  std::optional<std::string> serialize(const Ship& ship) const override;
  std::optional<Ship> deserialize(const std::string& json_str) const override;
};

// ShipRepository implementation
ShipRepository::ShipRepository(JsonStore& store)
    : Repository<Ship>(store, "tbl_ship") {}

std::optional<std::string> ShipRepository::serialize(const Ship& ship) const {
  auto result = glz::write_json(ship);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Ship> ShipRepository::deserialize(
    const std::string& json_str) const {
  Ship ship{};
  auto result = glz::read_json(ship, json_str);
  if (!result) {
    return ship;
  }
  return std::nullopt;
}

std::optional<Ship> ShipRepository::find_by_number(shipnum_t num) {
  return find(num);
}

bool ShipRepository::save_ship(const Ship& ship) {
  return save(ship.number, ship);
}

void ShipRepository::delete_ship(shipnum_t num) { remove(num); }

shipnum_t ShipRepository::next_ship_number() { return next_available_id(); }

shipnum_t ShipRepository::count_all_ships() {
  return static_cast<shipnum_t>(list_ids().size());
}

// Glaze reflection for Planet and related types
namespace glz {
template <>
struct meta<plroute> {
  using T = plroute;
  static constexpr auto value =
      object("set", &T::set, "dest_star", &T::dest_star, "dest_planet",
             &T::dest_planet, "load", &T::load, "unload", &T::unload, "x",
             &T::x, "y", &T::y);
};

template <>
struct meta<plinfo> {
  using T = plinfo;
  static constexpr auto value = object(
      "fuel", &T::fuel, "destruct", &T::destruct, "resource", &T::resource,
      "popn", &T::popn, "troops", &T::troops, "crystals", &T::crystals,
      "prod_res", &T::prod_res, "prod_fuel", &T::prod_fuel, "prod_dest",
      &T::prod_dest, "prod_crystals", &T::prod_crystals, "prod_money",
      &T::prod_money, "prod_tech", &T::prod_tech, "tech_invest",
      &T::tech_invest, "numsectsowned", &T::numsectsowned, "comread",
      &T::comread, "mob_set", &T::mob_set, "tox_thresh", &T::tox_thresh,
      "explored", &T::explored, "autorep", &T::autorep, "tax", &T::tax,
      "newtax", &T::newtax, "guns", &T::guns, "route", &T::route, "mob_points",
      &T::mob_points, "est_production", &T::est_production);
};

template <>
struct meta<Planet> {
  using T = Planet;
  static constexpr auto value =
      object("xpos", &T::xpos, "ypos", &T::ypos, "ships", &T::ships, "Maxx",
             &T::Maxx, "Maxy", &T::Maxy, "info", &T::info, "conditions",
             &T::conditions, "popn", &T::popn, "troops", &T::troops, "maxpopn",
             &T::maxpopn, "total_resources", &T::total_resources, "slaved_to",
             &T::slaved_to, "type", &T::type, "expltimer", &T::expltimer,
             "explored", &T::explored, "planet_id", &T::planet_id);
};
}  // namespace glz

// PlanetRepository - provides type-safe access to Planet entities
// Planets are stored with composite key (star_id, planet_order)
export class PlanetRepository : public Repository<Planet> {
 public:
  PlanetRepository(JsonStore& store);

  // Domain-specific methods
  // Note: Planets use composite keys (star_id, planet_order) in database
  std::optional<Planet> find_by_location(starnum_t star, planetnum_t pnum);
  bool save_at_location(const Planet& planet, starnum_t star, planetnum_t pnum);

 protected:
  std::optional<std::string> serialize(const Planet& planet) const override;
  std::optional<Planet> deserialize(const std::string& json_str) const override;
};

// PlanetRepository implementation
PlanetRepository::PlanetRepository(JsonStore& store)
    : Repository<Planet>(store, "tbl_planet") {}

std::optional<std::string> PlanetRepository::serialize(
    const Planet& planet) const {
  auto result = glz::write_json(planet);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Planet> PlanetRepository::deserialize(
    const std::string& json_str) const {
  Planet planet{};
  auto result = glz::read_json(planet, json_str);
  if (!result) {
    return planet;
  }
  return std::nullopt;
}

std::optional<Planet> PlanetRepository::find_by_location(starnum_t star,
                                                         planetnum_t pnum) {
  // Use multi-key retrieval: WHERE star_id=? AND planet_order=?
  std::vector<std::pair<std::string, int>> keys = {{"star_id", star},
                                                   {"planet_order", pnum}};
  auto json = store.retrieve_multi(table_name, keys);
  if (!json) return std::nullopt;
  return deserialize(*json);
}

bool PlanetRepository::save_at_location(const Planet& planet, starnum_t star,
                                        planetnum_t pnum) {
  auto json = serialize(planet);
  if (!json) return false;

  // Use multi-key storage: id, star_id, planet_order
  // The table has id as primary key + star_id/planet_order for lookups
  std::vector<std::pair<std::string, int>> keys = {
      {"id", planet.planet_id}, {"star_id", star}, {"planet_order", pnum}};
  return store.store_multi(table_name, keys, *json);
}

// Glaze reflection for star_struct
namespace glz {
template <>
struct meta<star_struct> {
  using T = star_struct;
  static constexpr auto value =
      object("ships", &T::ships, "name", &T::name, "governor", &T::governor,
             "AP", &T::AP, "explored", &T::explored, "inhabited", &T::inhabited,
             "xpos", &T::xpos, "ypos", &T::ypos, "numplanets", &T::numplanets,
             "pnames", &T::pnames, "stability", &T::stability, "nova_stage",
             &T::nova_stage, "temperature", &T::temperature, "gravity",
             &T::gravity, "star_id", &T::star_id, "dummy", &T::dummy);
};
}  // namespace glz

// StarRepository - provides type-safe access to Star entities
export class StarRepository : public Repository<star_struct> {
 public:
  StarRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<star_struct> find_by_number(starnum_t num);
  bool save_star(const star_struct& star, starnum_t num);

 protected:
  std::optional<std::string> serialize(
      const star_struct& star) const override;
  std::optional<star_struct> deserialize(
      const std::string& json_str) const override;
};

// StarRepository implementation
StarRepository::StarRepository(JsonStore& store)
    : Repository<star_struct>(store, "tbl_star") {}

std::optional<std::string> StarRepository::serialize(
    const star_struct& star) const {
  auto result = glz::write_json(star);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<star_struct> StarRepository::deserialize(
    const std::string& json_str) const {
  star_struct star{};
  auto result = glz::read_json(star, json_str);
  if (!result) {
    return star;
  }
  return std::nullopt;
}

std::optional<star_struct> StarRepository::find_by_number(starnum_t num) {
  return find(num);
}

bool StarRepository::save_star(const star_struct& star, starnum_t num) {
  return save(num, star);
}

// Glaze reflection for Sector
namespace glz {
template <>
struct meta<Sector> {
  using T = Sector;
  static constexpr auto value = object(
      "x", &T::x, "y", &T::y, "eff", &T::eff, "fert", &T::fert, "mobilization",
      &T::mobilization, "crystals", &T::crystals, "resource", &T::resource,
      "popn", &T::popn, "troops", &T::troops, "owner", &T::owner, "race",
      &T::race, "type", &T::type, "condition", &T::condition);
};
}  // namespace glz

// SectorRepository - provides type-safe access to Sector entities
// Note: Sectors use composite keys (planet_id, xpos, ypos) in database
export class SectorRepository : public Repository<Sector> {
 public:
  SectorRepository(JsonStore& store);

  // Domain-specific methods for individual sectors
  std::optional<Sector> find_sector(int planet_id, int x, int y);
  bool save_sector(const Sector& sector, int planet_id, int x, int y);

  // Bulk operations for sector maps
  SectorMap load_map(const Planet& planet);
  bool save_map(const SectorMap& map, const Planet& planet);

 protected:
  std::optional<std::string> serialize(const Sector& sector) const override;
  std::optional<Sector> deserialize(const std::string& json_str) const override;
};

// SectorRepository implementation
SectorRepository::SectorRepository(JsonStore& store)
    : Repository<Sector>(store, "tbl_sector") {}

std::optional<std::string> SectorRepository::serialize(
    const Sector& sector) const {
  auto result = glz::write_json(sector);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Sector> SectorRepository::deserialize(
    const std::string& json_str) const {
  Sector sector{};
  auto result = glz::read_json(sector, json_str);
  if (!result) {
    return sector;
  }
  return std::nullopt;
}

std::optional<Sector> SectorRepository::find_sector(int planet_id, int x,
                                                    int y) {
  // Use multi-key retrieval: WHERE planet_id=? AND xpos=? AND ypos=?
  std::vector<std::pair<std::string, int>> keys = {
      {"planet_id", planet_id}, {"xpos", x}, {"ypos", y}};
  auto json = store.retrieve_multi(table_name, keys);
  if (!json) return std::nullopt;
  return deserialize(*json);
}

bool SectorRepository::save_sector(const Sector& sector, int planet_id, int x,
                                   int y) {
  auto json = serialize(sector);
  if (!json) return false;

  // Use multi-key storage: planet_id, xpos, ypos
  std::vector<std::pair<std::string, int>> keys = {
      {"planet_id", planet_id}, {"xpos", x}, {"ypos", y}};
  return store.store_multi(table_name, keys, *json);
}

SectorMap SectorRepository::load_map(const Planet& planet) {
  SectorMap smap(planet);

  // Retrieve all sectors for this planet, ordered by position
  // This requires a custom SQL query, so we'll use the store's underlying
  // database For now, we'll load sectors individually
  for (int y = 0; y < planet.Maxy; y++) {
    for (int x = 0; x < planet.Maxx; x++) {
      auto sector = find_sector(planet.planet_id, x, y);
      if (sector.has_value()) {
        smap.put(std::move(*sector));
      }
    }
  }

  return smap;
}

bool SectorRepository::save_map(const SectorMap& map, const Planet& planet) {
  // Save all sectors in the map
  bool all_saved = true;
  for (int y = 0; y < planet.Maxy; y++) {
    for (int x = 0; x < planet.Maxx; x++) {
      const auto& sector = map.get(x, y);
      if (!save_sector(sector, planet.planet_id, x, y)) {
        all_saved = false;
      }
    }
  }
  return all_saved;
}
