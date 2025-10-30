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
