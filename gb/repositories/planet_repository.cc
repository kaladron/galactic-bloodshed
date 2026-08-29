// SPDX-License-Identifier: Apache-2.0

/// \file planet_repository.cc
/// \brief Implementation of PlanetRepository and Glaze reflection metadata for
/// Planet entities.

module;

import strong_id;
import glaze.core;
import glaze.json;

module gblib;

import dallib;
import std;

namespace glz {

template <>
struct meta<CommodityManifest> {
  using T = CommodityManifest;
  static constexpr auto value =
      object("fuel", &T::fuel, "destruct", &T::destruct, "resources",
             &T::resources, "crystals", &T::crystals);
};

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

PlanetRepository::PlanetRepository(JsonStore& store)
    : Repository<Planet>(store, "tbl_planet") {}

std::optional<std::string>
PlanetRepository::serialize(const Planet& planet) const {
  planet_struct data = planet.get_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Planet>
PlanetRepository::deserialize(const std::string& json_str) const {
  planet_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Planet(data);
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

  return store.store_multi(table_name,
                           {{"star_id", star}, {"planet_order", pnum}}, *json);
}
