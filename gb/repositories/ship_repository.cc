// SPDX-License-Identifier: Apache-2.0

/// \file ship_repository.cc
/// \brief Implementation of ShipRepository and Glaze reflection metadata for
/// Ship entities.

module;

import strong_id;
import glaze.core;
import glaze.json;

module gblib;

import dallib;
import std;

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

template <>
struct meta<NavigateData> {
  using T = NavigateData;
  static constexpr auto value =
      object("on", &T::on, "speed", &T::speed, "turns", &T::turns, "bearing",
             &T::bearing);
};

template <>
struct meta<ProtectData> {
  using T = ProtectData;
  static constexpr auto value =
      object("maxrng", &T::maxrng, "on", &T::on, "planet", &T::planet, "self",
             &T::self, "evade", &T::evade, "ship", &T::ship);
};

template <>
struct meta<HyperDriveData> {
  using T = HyperDriveData;
  static constexpr auto value = object("charge", &T::charge, "ready", &T::ready,
                                       "on", &T::on, "has", &T::has);
};

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

ShipRepository::ShipRepository(JsonStore& store)
    : Repository<Ship>(store, "tbl_ship") {}

std::optional<std::string> ShipRepository::serialize(const Ship& ship) const {
  ship_struct data = ship.get_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Ship>
ShipRepository::deserialize(const std::string& json_str) const {
  ship_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Ship(data);
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
