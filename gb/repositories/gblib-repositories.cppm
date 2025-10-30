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
