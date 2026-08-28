// SPDX-License-Identifier: Apache-2.0

/// \file sector_repository.cc
/// \brief Implementation of SectorRepository and Glaze reflection metadata for
/// Sector entities.

module;

import strong_id;
import glaze.core;
import glaze.json;

module gblib;

import dallib;
import std;

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

SectorRepository::SectorRepository(JsonStore& store)
    : Repository<Sector>(store, "tbl_sector") {}

std::optional<std::string>
SectorRepository::serialize(const Sector& sector) const {
  const sector_struct& data = sector.to_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Sector>
SectorRepository::deserialize(const std::string& json_str) const {
  sector_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Sector(data);
  }
  return std::nullopt;
}

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
    return;
  }

  store.store_multi(table_name, sector_keys(star_id, planet_order, coords),
                    *result);
}

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
  bool all_saved = true;
  for (auto [coord, sector] : map.indexed_dirty_sectors()) {
    if (!save_sector(sector, map.star_id(), map.planet_order(), coord.x,
                     coord.y)) {
      all_saved = false;
    }
  }
  return all_saved;
}
