// SPDX-License-Identifier: Apache-2.0

/// \file star_repository.cc
/// \brief Implementation of StarRepository and Glaze reflection metadata for
/// Star entities.

module;

import strong_id;
import glaze.core;
import glaze.json;

module gblib;

import dallib;
import std;

namespace glz {

template <>
struct meta<star_struct> {
  using T = star_struct;
  static constexpr auto value = object(
      "ships", &T::ships, "name", &T::name, "governor", &T::governor, "AP",
      &T::AP, "explored", &T::explored, "inhabited", &T::inhabited, "xpos",
      &T::xpos, "ypos", &T::ypos, "pnames", &T::pnames, "stability",
      &T::stability, "nova_stage", &T::nova_stage, "temperature",
      &T::temperature, "gravity", &T::gravity, "star_id", &T::star_id);
};

}  // namespace glz

StarRepository::StarRepository(JsonStore& store)
    : Repository<Star>(store, "tbl_star") {}

std::optional<std::string> StarRepository::serialize(const Star& star) const {
  star_struct data = star.get_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Star>
StarRepository::deserialize(const std::string& json_str) const {
  star_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Star(data);
  }
  return std::nullopt;
}

std::optional<Star> StarRepository::find_by_number(starnum_t num) {
  return find(num);
}

bool StarRepository::save(const Star& star) {
  auto star_struct_data = star.get_struct();
  return Repository<Star>::save(star_struct_data.star_id, star);
}
