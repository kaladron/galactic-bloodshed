// SPDX-License-Identifier: Apache-2.0

module;

import strong_id;
import glaze.core;
import glaze.json;

export module gblib:repositories;

import dallib;
import :types;
import :race;
import :ships;
import :star;
import :planet;
import :sector;
import :universe;
import std;

// Base template for repositories
// Provides common CRUD operations for entity types
// Derived classes must implement serialize/deserialize for their specific type
export template <typename T>
class Repository {
protected:
  JsonStore& store;
  std::string table_name;

  // Derived classes must implement these for their specific type
  [[nodiscard]] virtual std::optional<std::string>
  serialize(const T& entity) const = 0;
  [[nodiscard]] virtual std::optional<T>
  deserialize(const std::string& json) const = 0;

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
  bool save(const KeyValue& id, const T& entity) {
    if (auto json = serialize(entity)) {
      return store.store(table_name, id, *json);
    }
    return false;
  }

  // Find entity by ID
  std::optional<T> find(const KeyValue& id) {
    return store.retrieve(table_name, id).and_then([this](const auto& json) {
      return deserialize(json);
    });
  }

  // Find entity by composite keys
  std::optional<T>
  find_multi(const std::vector<std::pair<std::string, KeyValue>>& keys) {
    return store.retrieve_multi(table_name, keys)
        .and_then([this](const auto& json) { return deserialize(json); });
  }

  // Remove entity by ID
  bool remove(const KeyValue& id) {
    return store.remove(table_name, id);
  }

  // Get next available ID
  int next_available_id() {
    return store.find_next_available_id(table_name);
  }

  // List all IDs in the table
  std::vector<int> list_ids() {
    return store.list_ids(table_name);
  }
};

// Glaze reflection for Race (must be in global glz namespace)
namespace glz {
// Glaze reflection for strong ID types - serialize as underlying value
// This allows ID<Tag, T> to serialize/deserialize as plain integers
// Added BEFORE changing governor_t to ensure serialization works
template <FixedString Tag, typename T>
struct from<JSON, ID<Tag, T>> {
  template <auto Opts>
  static void op(ID<Tag, T>& id, is_context auto&& ctx, auto&& it, auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    id = ID<Tag, T>{val};
  }
};

template <FixedString Tag, typename T>
struct to<JSON, ID<Tag, T>> {
  template <auto Opts>
  static void op(const ID<Tag, T>& id, is_context auto&& ctx, auto&& b,
                 auto&& ix) {
    serialize<JSON>::op<Opts>(id.value, ctx, b, ix);
  }
};

template <>
struct meta<Coordinates> {
  using T = Coordinates;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct meta<toggletype> {
  using T = toggletype;
  static constexpr auto value =
      object("invisible", &T::invisible, "standby", &T::standby, "color",
             &T::color, "gag", &T::gag, "double_digits", &T::double_digits,
             "inverse", &T::inverse, "geography", &T::geography, "autoload",
             &T::autoload, "highlight", &T::highlight, "compat", &T::compat);
};

// Glaze reflection for Commod
template <>
struct meta<Commod> {
  using T = Commod;
  static constexpr auto value =
      object("id", &T::id, "owner", &T::owner, "governor", &T::governor, "type",
             &T::type, "amount", &T::amount, "deliver", &T::deliver, "bid",
             &T::bid, "bidder", &T::bidder, "bidder_gov", &T::bidder_gov,
             "star_from", &T::star_from, "planet_from", &T::planet_from,
             "star_to", &T::star_to, "planet_to", &T::planet_to);
};

// Glaze reflection for universe_struct
template <>
struct meta<universe_struct> {
  using T = universe_struct;
  static constexpr auto value =
      object("id", &T::id, "numstars", &T::numstars, "ships", &T::ships, "AP",
             &T::AP, "VN_hitlist", &T::VN_hitlist, "VN_index1", &T::VN_index1,
             "VN_index2", &T::VN_index2);
};

// Glaze reflection for block
template <>
struct meta<block> {
  using T = block;
  static constexpr auto value =
      object("Playernum", &T::Playernum, "name", &T::name, "motto", &T::motto,
             "invite", &T::invite, "pledge", &T::pledge, "atwar", &T::atwar,
             "allied", &T::allied, "next", &T::next, "systems_owned",
             &T::systems_owned, "VPs", &T::VPs, "money", &T::money);
};

// Glaze reflection for power
template <>
struct meta<power> {
  using T = power;
  static constexpr auto value =
      object("id", &T::id, "troops", &T::troops, "popn", &T::popn, "resource",
             &T::resource, "fuel", &T::fuel, "destruct", &T::destruct,
             "ships_owned", &T::ships_owned, "planets_owned", &T::planets_owned,
             "sectors_owned", &T::sectors_owned, "money", &T::money, "sum_mob",
             &T::sum_mob, "sum_eff", &T::sum_eff);
};

// Glaze reflection for ServerState
template <>
struct meta<ServerState> {
  using T = ServerState;
  static constexpr auto value =
      object("id", &T::id, "segments", &T::segments, "next_update_time",
             &T::next_update_time, "next_segment_time", &T::next_segment_time,
             "update_time_minutes", &T::update_time_minutes, "nsegments_done",
             &T::nsegments_done, "welcome_message", &T::welcome_message);
};

// Glaze reflection for ShipExam
template <>
struct meta<ShipExam> {
  using T = ShipExam;
  static constexpr auto value =
      object("ship_type", &T::ship_type, "name", &T::name, "description",
             &T::description);
};
}  // namespace glz

// RaceRepository - provides type-safe access to Race entities
export class RaceRepository : public Repository<Race> {
public:
  RaceRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Race> find_by_player(player_t player);
  bool save(const Race& race);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Race& race) const override;
  [[nodiscard]] std::optional<Race>
  deserialize(const std::string& json_str) const override;
};

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

// Glaze reflection for NavigateData
template <>
struct meta<NavigateData> {
  using T = NavigateData;
  static constexpr auto value =
      object("on", &T::on, "speed", &T::speed, "turns", &T::turns, "bearing",
             &T::bearing);
};

// Glaze reflection for ProtectData
template <>
struct meta<ProtectData> {
  using T = ProtectData;
  static constexpr auto value =
      object("maxrng", &T::maxrng, "on", &T::on, "planet", &T::planet, "self",
             &T::self, "evade", &T::evade, "ship", &T::ship);
};

// Glaze reflection for HyperDriveData
template <>
struct meta<HyperDriveData> {
  using T = HyperDriveData;
  static constexpr auto value = object("charge", &T::charge, "ready", &T::ready,
                                       "on", &T::on, "has", &T::has);
};

// Glaze reflection for ship_struct (POD for serialization)
template <>
struct meta<ship_struct> {
  using T = ship_struct;
  static constexpr auto value = object(
      "number", &T::number, "owner", &T::owner, "governor", &T::governor,
      "name", &T::name, "shipclass", &T::shipclass, "race", &T::race, "xpos",
      &T::xpos, "ypos", &T::ypos, "fuel", &T::fuel, "mass", &T::mass,
      "land_coords", &T::land_coords, "destshipno", &T::destshipno, "nextship",
      &T::nextship, "ships", &T::ships, "armor", &T::armor, "size", &T::size,
      "max_crew", &T::max_crew, "max_resource", &T::max_resource,
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
  bool save(const Ship& ship);
  void delete_ship(shipnum_t num);
  shipnum_t next_ship_number();
  shipnum_t count_all_ships();

  // Spatial and indexed query methods
  [[nodiscard]] std::vector<shipnum_t>
  find_in_star_system(starnum_t star_id, bool alive_only = true);
  [[nodiscard]] std::vector<shipnum_t> find_in_star(starnum_t star_id,
                                                    bool alive_only = true);
  [[nodiscard]] std::vector<shipnum_t> find_on_planet(starnum_t star_id,
                                                      planetnum_t planet_id,
                                                      bool alive_only = true);
  [[nodiscard]] std::vector<shipnum_t> find_in_hangar(shipnum_t carrier_id,
                                                      bool alive_only = true);
  [[nodiscard]] std::vector<shipnum_t> find_by_owner(player_t owner_id,
                                                     bool alive_only = true);
  [[nodiscard]] std::vector<shipnum_t> find_at_scope(ScopeLevel scope,
                                                     bool alive_only = true);
  [[nodiscard]] std::vector<shipnum_t> find_alive();

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Ship& ship) const override;
  [[nodiscard]] std::optional<Ship>
  deserialize(const std::string& json_str) const override;
};

// ShipRepository implementation
ShipRepository::ShipRepository(JsonStore& store)
    : Repository<Ship>(store, "tbl_ship") {}

std::optional<std::string> ShipRepository::serialize(const Ship& ship) const {
  // Extract ship_struct from Ship wrapper
  ship_struct data = ship.get_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Ship>
ShipRepository::deserialize(const std::string& json_str) const {
  // Deserialize to ship_struct, then wrap in Ship
  ship_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Ship(data);  // Wrap the ship_struct in Ship
  }
  return std::nullopt;
}

std::optional<Ship> ShipRepository::find_by_number(shipnum_t num) {
  return find(num);
}

bool ShipRepository::save(const Ship& ship) {
  return Repository<Ship>::save(ship.number(), ship);
}

void ShipRepository::delete_ship(shipnum_t num) {
  remove(num);
}

shipnum_t ShipRepository::next_ship_number() {
  return next_available_id();
}

shipnum_t ShipRepository::count_all_ships() {
  return static_cast<shipnum_t>(list_ids().size());
}

std::vector<shipnum_t> ShipRepository::find_in_star_system(starnum_t star_id,
                                                           bool alive_only) {
  std::string where = "storbits = ?";
  std::vector<KeyValue> params{star_id.value};
  if (alive_only) {
    where += " AND alive = 1";
  }
  where += " ORDER BY id";
  auto ids = store.query_ids(table_name, where, params);
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

std::vector<shipnum_t> ShipRepository::find_in_star(starnum_t star_id,
                                                    bool alive_only) {
  std::string where = "storbits = ? AND whatorbits = ?";
  std::vector<KeyValue> params{star_id.value,
                               static_cast<int>(ScopeLevel::LEVEL_STAR)};
  if (alive_only) {
    where += " AND alive = 1";
  }
  where += " ORDER BY id";
  auto ids = store.query_ids(table_name, where, params);
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

std::vector<shipnum_t> ShipRepository::find_on_planet(starnum_t star_id,
                                                      planetnum_t planet_id,
                                                      bool alive_only) {
  std::string where = "storbits = ? AND pnumorbits = ?";
  std::vector<KeyValue> params{star_id.value, planet_id.value};
  if (alive_only) {
    where += " AND alive = 1";
  }
  where += " ORDER BY id";
  auto ids = store.query_ids(table_name, where, params);
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

std::vector<shipnum_t> ShipRepository::find_in_hangar(shipnum_t carrier_id,
                                                      bool alive_only) {
  std::string where = "destshipno = ? AND whatorbits = ?";
  std::vector<KeyValue> params{carrier_id.value,
                               static_cast<int>(ScopeLevel::LEVEL_SHIP)};
  if (alive_only) {
    where += " AND alive = 1";
  }
  where += " ORDER BY id";
  auto ids = store.query_ids(table_name, where, params);
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

std::vector<shipnum_t> ShipRepository::find_by_owner(player_t owner_id,
                                                     bool alive_only) {
  std::string where = "owner = ?";
  std::vector<KeyValue> params{owner_id.value};
  if (alive_only) {
    where += " AND alive = 1";
  }
  where += " ORDER BY id";
  auto ids = store.query_ids(table_name, where, params);
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

std::vector<shipnum_t> ShipRepository::find_at_scope(ScopeLevel scope,
                                                     bool alive_only) {
  std::string where = "whatorbits = ?";
  std::vector<KeyValue> params{static_cast<int>(scope)};
  if (alive_only) {
    where += " AND alive = 1";
  }
  where += " ORDER BY id";
  auto ids = store.query_ids(table_name, where, params);
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

std::vector<shipnum_t> ShipRepository::find_alive() {
  auto ids = store.query_ids(table_name, "alive = 1 ORDER BY id", {});
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    result.emplace_back(id);
  }
  return result;
}

// Glaze reflection for Planet and related types
namespace glz {
template <>
struct meta<plroute> {
  using T = plroute;
  static constexpr auto value =
      object("set", &T::set, "dest_star", &T::dest_star, "dest_planet",
             &T::dest_planet, "load", &T::load, "unload", &T::unload,
             "dest_coords", &T::dest_coords);
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

template <typename T, std::size_t N>
struct meta<PlayerVector<T, N>> {
  using Type = PlayerVector<T, N>;
  static constexpr auto value = [](auto&& self) -> auto& {
    return self.raw_array();
  };
};

template <>
struct meta<planet_struct> {
  using T = planet_struct;
  static constexpr auto value = object(
      "xpos", &T::xpos, "ypos", &T::ypos, "ships", &T::ships, "dimensions",
      &T::dimensions, "info", &T::info, "conditions", &T::conditions, "popn",
      &T::popn, "troops", &T::troops, "maxpopn", &T::maxpopn, "total_resources",
      &T::total_resources, "slaved_to", &T::slaved_to, "type", &T::type,
      "expltimer", &T::expltimer, "explored", &T::explored, "star_id",
      &T::star_id, "planet_order", &T::planet_order);
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
  bool save(const Planet& planet);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Planet& planet) const override;
  [[nodiscard]] std::optional<Planet>
  deserialize(const std::string& json_str) const override;

private:
  // Helper for internal use with explicit parameters
  bool save_planet_impl(const Planet& planet, starnum_t star, planetnum_t pnum);
};

// PlanetRepository implementation
PlanetRepository::PlanetRepository(JsonStore& store)
    : Repository<Planet>(store, "tbl_planet") {}

std::optional<std::string>
PlanetRepository::serialize(const Planet& planet) const {
  // Extract planet_struct from Planet wrapper
  planet_struct data = planet.get_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Planet>
PlanetRepository::deserialize(const std::string& json_str) const {
  // Deserialize to planet_struct, then wrap in Planet
  planet_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Planet(data);  // Wrap the planet_struct in Planet
  }
  return std::nullopt;
}

std::optional<Planet> PlanetRepository::find_by_location(starnum_t star,
                                                         planetnum_t pnum) {
  return find_multi({{"star_id", star}, {"planet_order", pnum}});
}

bool PlanetRepository::save(const Planet& planet) {
  return save_planet_impl(planet, planet.star_id(), planet.planet_order());
}

bool PlanetRepository::save_planet_impl(const Planet& planet, starnum_t star,
                                        planetnum_t pnum) {
  auto json = serialize(planet);
  if (!json) return false;

  // Use composite key (star_id, planet_order) - no 'id' column
  return store.store_multi(table_name,
                           {{"star_id", star}, {"planet_order", pnum}}, *json);
}

// StarRepository - provides type-safe access to Star entities
export class StarRepository : public Repository<Star> {
public:
  StarRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Star> find_by_number(starnum_t num);
  bool save(const Star& star);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Star& star) const override;
  [[nodiscard]] std::optional<Star>
  deserialize(const std::string& json_str) const override;
};

// Glaze reflection for sector_struct (POD)
namespace glz {
template <>
struct meta<sector_struct> {
  using T = sector_struct;
  static constexpr auto value = object(
      "coords", &T::coords, "eff", &T::eff, "fert", &T::fert, "mobilization",
      &T::mobilization, "crystals", &T::crystals, "resource", &T::resource,
      "popn", &T::popn, "troops", &T::troops, "owner", &T::owner, "race",
      &T::race, "type", &T::type, "condition", &T::condition);
};
}  // namespace glz

// SectorRepository - provides type-safe access to Sector entities
// Note: Sectors use composite keys (star_id, planet_order, xpos, ypos) in
// database
export class SectorRepository : public Repository<Sector> {
public:
  SectorRepository(JsonStore& store);

  // Domain-specific methods working with sector_struct and Coordinates
  [[nodiscard]] sector_struct load(starnum_t star_id, planetnum_t planet_order,
                                   Coordinates coords);
  [[nodiscard]] sector_struct load(starnum_t star_id, planetnum_t planet_order,
                                   std::size_t x, std::size_t y) {
    return load(star_id, planet_order,
                Coordinates{static_cast<int>(x), static_cast<int>(y)});
  }

  void save(starnum_t star_id, planetnum_t planet_order, Coordinates coords,
            const sector_struct& sector);
  void save(starnum_t star_id, planetnum_t planet_order, std::size_t x,
            std::size_t y, const sector_struct& sector) {
    save(star_id, planet_order,
         Coordinates{static_cast<int>(x), static_cast<int>(y)}, sector);
  }

  // Legacy methods (for backward compatibility during migration)
  std::optional<Sector> find_sector(starnum_t star_id, planetnum_t planet_order,
                                    Coordinates coords);
  std::optional<Sector> find_sector(starnum_t star_id, planetnum_t planet_order,
                                    int x, int y) {
    return find_sector(star_id, planet_order, Coordinates{x, y});
  }

  bool save_sector(const Sector& sector, starnum_t star_id,
                   planetnum_t planet_order, Coordinates coords);
  bool save_sector(const Sector& sector, starnum_t star_id,
                   planetnum_t planet_order, int x, int y) {
    return save_sector(sector, star_id, planet_order, Coordinates{x, y});
  }

  // Bulk operations for sector maps
  SectorMap load_map(const Planet& planet);
  bool save_map(const SectorMap& map);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Sector& sector) const override;
  [[nodiscard]] std::optional<Sector>
  deserialize(const std::string& json_str) const override;

private:
  static std::vector<std::pair<std::string, KeyValue>>
  sector_keys(starnum_t star_id, planetnum_t planet_order, Coordinates coords) {
    return {{"star_id", star_id},
            {"planet_order", planet_order},
            {"xpos", coords.x},
            {"ypos", coords.y}};
  }
};

// SectorRepository implementation
SectorRepository::SectorRepository(JsonStore& store)
    : Repository<Sector>(store, "tbl_sector") {}

std::optional<std::string>
SectorRepository::serialize(const Sector& sector) const {
  // Serialize the underlying sector_struct
  const sector_struct& data = sector.to_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Sector>
SectorRepository::deserialize(const std::string& json_str) const {
  // Deserialize to sector_struct, then wrap in Sector
  sector_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Sector(data);
  }
  return std::nullopt;
}

// Methods working directly with sector_struct & Coordinates
sector_struct SectorRepository::load(starnum_t star_id,
                                     planetnum_t planet_order,
                                     Coordinates coords) {
  if (auto json = store.retrieve_multi(
          table_name, sector_keys(star_id, planet_order, coords))) {
    sector_struct data{};
    if (!glz::read_json(data, *json)) {
      return data;
    }
  }
  return sector_struct{};
}

void SectorRepository::save(starnum_t star_id, planetnum_t planet_order,
                            Coordinates coords, const sector_struct& sector) {
  auto result = glz::write_json(sector);
  if (!result.has_value()) {
    return;  // Serialization failed
  }

  store.store_multi(table_name, sector_keys(star_id, planet_order, coords),
                    *result);
}

// Legacy methods (for backward compatibility)
std::optional<Sector> SectorRepository::find_sector(starnum_t star_id,
                                                    planetnum_t planet_order,
                                                    Coordinates coords) {
  return find_multi(sector_keys(star_id, planet_order, coords));
}

bool SectorRepository::save_sector(const Sector& sector, starnum_t star_id,
                                   planetnum_t planet_order,
                                   Coordinates coords) {
  auto json = serialize(sector);
  if (!json) return false;

  return store.store_multi(table_name,
                           sector_keys(star_id, planet_order, coords), *json);
}

SectorMap SectorRepository::load_map(const Planet& planet) {
  SectorMap smap(planet);

  // Retrieve all sectors for this planet, ordered by position
  // This requires a custom SQL query, so we'll use the store's underlying
  // database For now, we'll load sectors individually
  for (int y = 0; y < planet.dimensions().y; y++) {
    for (int x = 0; x < planet.dimensions().x; x++) {
      if (auto sector =
              find_sector(planet.star_id(), planet.planet_order(), x, y)) {
        smap.set(Coordinates{x, y}, std::move(*sector));
      }
    }
  }

  smap.clear_dirty();
  return smap;
}

bool SectorRepository::save_map(const SectorMap& map) {
  // Save all dirty sectors in the map using map's stored planet identity
  bool all_saved = true;
  for (auto [coord, sector] : map.indexed_dirty_sectors()) {
    if (!save_sector(sector, map.star_id(), map.planet_order(), coord.x,
                     coord.y)) {
      all_saved = false;
    }
  }
  return all_saved;
}

// ============================================================================
// CommodRepository - Repository for commodity market data
// ============================================================================
export class CommodRepository : public Repository<Commod> {
public:
  explicit CommodRepository(JsonStore& store)
      : Repository<Commod>(store, "tbl_commod") {}

  // Domain-specific methods
  std::optional<Commod> find_by_id(int id) {
    return find(id);
  }
  bool save(const Commod& commod) {
    return Repository<Commod>::save(commod.id, commod);
  }
  void delete_commod(int id) {
    store.remove(table_name, id);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Commod& commod) const override {
    auto result = glz::write_json(commod);
    if (result.has_value()) {
      return result.value();
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<Commod>
  deserialize(const std::string& json_str) const override {
    Commod commod{};
    auto result = glz::read_json(commod, json_str);
    if (!result) {
      return commod;
    }
    return std::nullopt;
  }
};

// ============================================================================
// BlockRepository - Repository for alliance block data
// ============================================================================
export class BlockRepository : public Repository<block> {
public:
  explicit BlockRepository(JsonStore& store)
      : Repository<block>(store, "tbl_block") {}

  // Domain-specific methods
  std::optional<block> find_by_id(blocknum_t id) {
    return find(id);
  }
  bool save(const block& b) {
    return Repository<block>::save(b.Playernum.value, b);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const block& b) const override {
    auto result = glz::write_json(b);
    if (result.has_value()) {
      return result.value();
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<block>
  deserialize(const std::string& json_str) const override {
    block b{};
    auto result = glz::read_json(b, json_str);
    if (!result) {
      return b;
    }
    return std::nullopt;
  }
};

// ============================================================================
// PowerRepository - Repository for player power statistics
// ============================================================================
export class PowerRepository : public Repository<power> {
public:
  explicit PowerRepository(JsonStore& store)
      : Repository<power>(store, "tbl_power") {}

  // Domain-specific methods
  std::optional<power> find_by_id(powernum_t id) {
    return find(id);
  }
  bool save(const power& p) {
    return Repository<power>::save(p.id, p);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const power& p) const override {
    auto result = glz::write_json(p);
    if (result.has_value()) {
      return result.value();
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<power>
  deserialize(const std::string& json_str) const override {
    power p{};
    auto result = glz::read_json(p, json_str);
    if (!result) {
      return p;
    }
    return std::nullopt;
  }
};

// ============================================================================
// UniverseRepository - Repository for global universe-wide statistics
// ============================================================================
export class UniverseRepository : public Repository<universe_struct> {
public:
  explicit UniverseRepository(JsonStore& store)
      : Repository<universe_struct>(store, "tbl_universe") {}

  // Domain-specific methods
  // Note: universe_struct is a singleton (id=1)
  std::optional<universe_struct> get_global_data() {
    return find(1);
  }
  bool save(const universe_struct& universe) {
    return Repository<universe_struct>::save(universe.id, universe);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const universe_struct& universe) const override {
    auto result = glz::write_json(universe);
    if (result.has_value()) {
      return result.value();
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<universe_struct>
  deserialize(const std::string& json_str) const override {
    universe_struct universe{};
    auto result = glz::read_json(universe, json_str);
    if (!result) {
      return universe;
    }
    return std::nullopt;
  }
};

// ============================================================================
// ServerStateRepository - Repository for server scheduling state
// ============================================================================
export class ServerStateRepository : public Repository<ServerState> {
public:
  explicit ServerStateRepository(JsonStore& store)
      : Repository<ServerState>(store, "tbl_server_state") {}

  // Domain-specific methods
  // Note: ServerState is a singleton (id=1)
  std::optional<ServerState> get_state() {
    return find(1);
  }
  bool save(const ServerState& state) {
    return Repository<ServerState>::save(state.id, state);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const ServerState& state) const override {
    auto result = glz::write_json(state);
    if (result.has_value()) {
      return result.value();
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<ServerState>
  deserialize(const std::string& json_str) const override {
    ServerState state{};
    auto result = glz::read_json(state, json_str);
    if (!result) {
      return state;
    }
    return std::nullopt;
  }
};

// ============================================================================
// ShipExamRepository - Repository for ship examination descriptions
// ============================================================================
export class ShipExamRepository : public Repository<ShipExam> {
public:
  explicit ShipExamRepository(JsonStore& store)
      : Repository<ShipExam>(store, "tbl_ship_exam") {}

  // Domain-specific methods
  std::optional<ShipExam> find_by_type(ShipType ship_type) {
    return find(std::to_underlying(ship_type));
  }
  bool save(const ShipExam& exam) {
    return Repository<ShipExam>::save(std::to_underlying(exam.ship_type), exam);
  }

  bool seed_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
      return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    std::vector<std::string> sections;
    std::size_t start = 0;
    std::size_t end = content.find('~');
    while (end != std::string::npos) {
      sections.push_back(content.substr(start, end - start));
      start = end + 1;
      end = content.find('~', start);
    }
    if (start < content.size()) {
      sections.push_back(content.substr(start));
    }

    int type = 0;
    for (const auto& section : sections) {
      auto first = section.find_first_not_of(" \t\n\r");
      if (first == std::string::npos) continue;
      auto last = section.find_last_not_of(" \t\n\r");
      std::string trimmed = section.substr(first, (last - first + 1));

      if (type < NUMSTYPES) {
        auto stype = static_cast<ShipType>(type);
        ShipExam exam{.ship_type = stype,
                      .name = std::string(Shipnames[type]),
                      .description = trimmed};
        save(exam);
        type++;
      }
    }
    return type > 0;
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const ShipExam& exam) const override {
    auto result = glz::write_json(exam);
    if (result.has_value()) {
      return result.value();
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<ShipExam>
  deserialize(const std::string& json_str) const override {
    ShipExam exam{};
    auto result = glz::read_json(exam, json_str);
    if (!result) {
      return exam;
    }
    return std::nullopt;
  }
};

// ============================================================================
// NewsRepository - Repository for news/telegram items
// Delegates all SQL operations to the DAL layer
// ============================================================================
export class NewsRepository {
private:
  Database& db;

public:
  explicit NewsRepository(Database& database) : db(database) {}

  // Add news item and return auto-generated ID
  std::optional<int> add(NewsType type, std::string_view message) {
    auto now = std::chrono::system_clock::now();
    auto timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();

    return db.news_add(std::to_underlying(type), std::string(message),
                       timestamp);
  }

  // Get news items of specific type after a given ID (for pagination)
  std::vector<NewsItem> get_since(NewsType type, int since_id = 0) {
    auto tuples = db.news_get_since(std::to_underlying(type), since_id);

    std::vector<NewsItem> items;
    items.reserve(tuples.size());

    for (const auto& [id, type_val, message, timestamp] : tuples) {
      NewsItem item;
      item.id = id;
      item.type = type_val;  // NewsItem.type is now int
      item.message = message;
      item.timestamp = timestamp;
      items.push_back(std::move(item));
    }

    return items;
  }

  // Get latest news ID for a specific type (for tracking what user has read)
  int get_latest_id(NewsType type) {
    return db.news_get_latest_id(std::to_underlying(type));
  }

  // Delete all news of a specific type (for purge)
  bool purge_type(NewsType type) {
    return db.news_purge_type(std::to_underlying(type));
  }

  // Delete all news (for complete purge)
  bool purge_all() {
    return db.news_purge_all();
  }
};
// ============================================================================
// TelegramRepository - Repository for telegram items
// Delegates all SQL operations to the DAL layer
// ============================================================================
export struct TelegramItem {
  int id{0};
  player_t recipient_player{0};
  governor_t recipient_governor{0};
  std::string message;
  std::int64_t timestamp{0};
};

export class TelegramRepository {
private:
  Database& db;

public:
  explicit TelegramRepository(Database& database) : db(database) {}

  // Add telegram and return auto-generated ID
  std::optional<int> add(player_t player, governor_t governor,
                         std::string_view message) {
    auto now = std::chrono::system_clock::now();
    auto timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();

    return db.telegram_add(player, governor, std::string(message), timestamp);
  }

  // Get all telegrams for a specific recipient
  std::vector<TelegramItem> get(player_t player, governor_t governor) {
    auto tuples = db.telegram_get(player, governor);

    std::vector<TelegramItem> items;
    items.reserve(tuples.size());

    for (const auto& [id, recv_player, recv_governor, message, timestamp] :
         tuples) {
      TelegramItem item;
      item.id = id;
      item.recipient_player = recv_player;
      item.recipient_governor = recv_governor;
      item.message = message;
      item.timestamp = timestamp;
      items.push_back(std::move(item));
    }

    return items;
  }

  // Delete all telegrams for a specific governor (delete on read behavior)
  void delete_for_governor(player_t player, governor_t governor) {
    db.telegram_delete_for_governor(player, governor);
  }

  // Count telegrams for a specific recipient
  int count(player_t player, governor_t governor) {
    return db.telegram_count(player, governor);
  }

  // Delete all telegrams (for purge command)
  bool purge_all() {
    return db.telegram_purge_all();
  }
};
