// SPDX-License-Identifier: Apache-2.0

/// \file misc_repositories.cc
/// \brief Implementation of CommodRepository, BlockRepository, PowerRepository,
/// UniverseRepository, ServerStateRepository, and ShipExamRepository.

module;

import strong_id;
import glaze.core;
import glaze.json;

module gb.repositories;

import gb.repositories.glaze;
import dallib;
import gb.entities;
import std;

namespace glz {

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

template <>
struct meta<universe_struct> {
  using T = universe_struct;
  static constexpr auto value =
      object("id", &T::id, "numstars", &T::numstars, "ships", &T::ships, "AP",
             &T::AP, "VN_hitlist", &T::VN_hitlist, "VN_index1", &T::VN_index1,
             "VN_index2", &T::VN_index2);
};

template <>
struct meta<block> {
  using T = block;
  static constexpr auto value =
      object("Playernum", &T::Playernum, "name", &T::name, "motto", &T::motto,
             "invited", &T::invited, "pledged", &T::pledged, "atwar", &T::atwar,
             "allied", &T::allied, "members", &T::members, "troops", &T::troops,
             "popn", &T::popn, "resource", &T::resource, "fuel", &T::fuel,
             "destruct", &T::destruct, "ships_owned", &T::ships_owned,
             "systems_owned", &T::systems_owned, "sectors_owned",
             &T::sectors_owned, "VPs", &T::VPs, "money", &T::money);
};

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

template <>
struct meta<ServerState> {
  using T = ServerState;
  static constexpr auto value = object(
      "id", &T::id, "segments", &T::segments, "next_update_time",
      &T::next_update_time, "next_segment_time", &T::next_segment_time,
      "update_time_minutes", &T::update_time_minutes, "nsegments_done",
      &T::nsegments_done, "nupdates_done", &T::nupdates_done,
      "server_start_time", &T::server_start_time, "last_update_time",
      &T::last_update_time, "last_segment_time", &T::last_segment_time,
      "start_buf", &T::start_buf, "update_buf", &T::update_buf, "segment_buf",
      &T::segment_buf, "welcome_message", &T::welcome_message);
};

template <>
struct meta<ShipExam> {
  using T = ShipExam;
  static constexpr auto value =
      object("ship_type", &T::ship_type, "name", &T::name, "description",
             &T::description);
};

}  // namespace glz

std::optional<std::string>
CommodRepository::serialize(const Commod& commod) const {
  auto result = glz::write_json(commod);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Commod>
CommodRepository::deserialize(const std::string& json_str) const {
  Commod commod{};
  auto result = glz::read_json(commod, json_str);
  if (!result) {
    return commod;
  }
  return std::nullopt;
}

std::optional<std::string> BlockRepository::serialize(const block& b) const {
  auto result = glz::write_json(b);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<block>
BlockRepository::deserialize(const std::string& json_str) const {
  block b{};
  auto result = glz::read_json(b, json_str);
  if (!result) {
    return b;
  }
  return std::nullopt;
}

std::optional<std::string> PowerRepository::serialize(const power& p) const {
  auto result = glz::write_json(p);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<power>
PowerRepository::deserialize(const std::string& json_str) const {
  power p{};
  auto result = glz::read_json(p, json_str);
  if (!result) {
    return p;
  }
  return std::nullopt;
}

std::optional<std::string>
UniverseRepository::serialize(const universe_struct& universe) const {
  auto result = glz::write_json(universe);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<universe_struct>
UniverseRepository::deserialize(const std::string& json_str) const {
  universe_struct universe{};
  auto result = glz::read_json(universe, json_str);
  if (!result) {
    return universe;
  }
  return std::nullopt;
}

std::optional<std::string>
ServerStateRepository::serialize(const ServerState& state) const {
  auto result = glz::write_json(state);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<ServerState>
ServerStateRepository::deserialize(const std::string& json_str) const {
  ServerState state{};
  auto result = glz::read_json(state, json_str);
  if (!result) {
    return state;
  }
  return std::nullopt;
}

bool ShipExamRepository::seed_from_file(const std::string& path) {
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
                    .name = std::string(ship_template(stype).name),
                    .description = trimmed};
      save(exam);
      type++;
    }
  }
  return type > 0;
}

std::optional<std::string>
ShipExamRepository::serialize(const ShipExam& exam) const {
  auto result = glz::write_json(exam);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<ShipExam>
ShipExamRepository::deserialize(const std::string& json_str) const {
  ShipExam exam{};
  auto result = glz::read_json(exam, json_str);
  if (!result) {
    return exam;
  }
  return std::nullopt;
}
