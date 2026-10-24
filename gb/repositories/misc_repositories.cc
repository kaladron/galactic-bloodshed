// SPDX-License-Identifier: Apache-2.0

/// \file misc_repositories.cc
/// \brief Implementation of CommodRepository, BlockRepository, PowerRepository,
/// UniverseRepository, ServerStateRepository, ShipExamRepository,
/// NewsRepository, and TelegramRepository.

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
struct meta<commod_struct> {
  using T = commod_struct;
  static constexpr auto value =
      object("id", &T::id, "owner", &T::owner, "governor", &T::governor, "type",
             &T::type, "amount", &T::amount, "deliver", &T::deliver, "bid",
             &T::bid, "bidder", &T::bidder, "bidder_gov", &T::bidder_gov,
             "star_from", &T::star_from, "planet_from", &T::planet_from,
             "star_to", &T::star_to, "planet_to", &T::planet_to);
};

template <>
struct meta<VnTargetRecord> {
  using T = VnTargetRecord;
  static constexpr auto value =
      object("hits", &T::hits, "primary_star", &T::primary_star,
             "secondary_star", &T::secondary_star);
};

template <>
struct meta<universe_struct> {
  using T = universe_struct;
  static constexpr auto value =
      object("AP", &T::AP, "vn_targets", &T::vn_targets);
};

template <>
struct meta<block_struct> {
  using T = block_struct;
  static constexpr auto value = object(
      "Playernum", &T::Playernum, "name", &T::name, "motto", &T::motto,
      "invited", &T::invited, "pledged", &T::pledged, "members", &T::members,
      "popn", &T::popn, "resource", &T::resource, "fuel", &T::fuel, "destruct",
      &T::destruct, "ships_owned", &T::ships_owned, "systems_owned",
      &T::systems_owned, "VPs", &T::VPs, "money", &T::money);
};

template <>
struct meta<power_struct> {
  using T = power_struct;
  static constexpr auto value = object(
      "id", &T::id, "troops", &T::troops, "popn", &T::popn, "resource",
      &T::resource, "fuel", &T::fuel, "destruct", &T::destruct, "ships_owned",
      &T::ships_owned, "planets_owned", &T::planets_owned, "money", &T::money);
};

template <>
struct meta<ServerState> {
  using T = ServerState;
  static constexpr auto value =
      object("segments", &T::segments, "next_update_time", &T::next_update_time,
             "next_segment_time", &T::next_segment_time, "update_time_minutes",
             &T::update_time_minutes, "nsegments_done", &T::nsegments_done,
             "nupdates_done", &T::nupdates_done, "server_start_time",
             &T::server_start_time, "last_update_time", &T::last_update_time,
             "last_segment_time", &T::last_segment_time, "welcome_message",
             &T::welcome_message);
};

template <>
struct meta<ShipExam> {
  using T = ShipExam;
  static constexpr auto value =
      object("ship_type", &T::ship_type, "name", &T::name, "description",
             &T::description);
};

template <>
struct meta<NewsItem> {
  using T = NewsItem;
  static constexpr auto value =
      object("id", &T::id, "type", &T::type, "message", &T::message,
             "timestamp", &T::timestamp);
};

template <>
struct meta<TelegramItem> {
  using T = TelegramItem;
  static constexpr auto value =
      object("id", &T::id, "recipient_player", &T::recipient_player,
             "recipient_governor", &T::recipient_governor, "message",
             &T::message, "timestamp", &T::timestamp);
};

}  // namespace glz

std::optional<std::string>
CommodRepository::serialize(const Commod& commod) const {
  auto result = glz::write_json(commod.get_struct());
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Commod>
CommodRepository::deserialize(const std::string& json_str) const {
  commod_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Commod{data};
  }
  return std::nullopt;
}

std::optional<std::string> BlockRepository::serialize(const Block& b) const {
  auto result = glz::write_json(b.get_struct());
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Block>
BlockRepository::deserialize(const std::string& json_str) const {
  block_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Block{data};
  }
  return std::nullopt;
}

std::optional<std::string> PowerRepository::serialize(const Power& p) const {
  auto result = glz::write_json(p.get_struct());
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Power>
PowerRepository::deserialize(const std::string& json_str) const {
  power_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Power{data};
  }
  return std::nullopt;
}

std::optional<std::string>
UniverseRepository::serialize(const Universe& universe) const {
  auto result = glz::write_json(universe.get_struct());
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Universe>
UniverseRepository::deserialize(const std::string& json_str) const {
  universe_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Universe(data);
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

std::optional<std::string>
NewsRepository::serialize(const NewsItem& item) const {
  auto result = glz::write_json(item);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<NewsItem>
NewsRepository::deserialize(const std::string& json_str) const {
  NewsItem data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return data;
  }
  return std::nullopt;
}

news_id_t NewsRepository::add(NewsType type, std::string_view message) {
  const auto now = std::chrono::system_clock::now();
  const auto timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
  const news_id_t id{next_available_id()};
  const NewsItem item{
      .id = id,
      .type = type,
      .message = std::string(message),
      .timestamp = timestamp,
  };
  save(id, item);
  return id;
}

std::vector<NewsItem> NewsRepository::get_since(NewsType type,
                                                news_id_t since_id) {
  return find_where("type = ? AND id > ? ORDER BY timestamp ASC, id ASC",
                    {std::to_underlying(type), since_id});
}

bool NewsRepository::purge_type(NewsType type) {
  return remove_where("type = ?", {std::to_underlying(type)});
}

bool NewsRepository::purge_all() {
  return remove_where();
}

std::optional<std::string>
TelegramRepository::serialize(const TelegramItem& item) const {
  auto result = glz::write_json(item);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<TelegramItem>
TelegramRepository::deserialize(const std::string& json_str) const {
  TelegramItem data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return data;
  }
  return std::nullopt;
}

telegram_id_t TelegramRepository::add(player_t player, governor_t governor,
                                      std::string_view message) {
  const auto now = std::chrono::system_clock::now();
  const auto timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
  const telegram_id_t id{next_available_id()};
  const TelegramItem item{
      .id = id,
      .recipient_player = player,
      .recipient_governor = governor,
      .message = std::string(message),
      .timestamp = timestamp,
  };
  save(id, item);
  return id;
}

std::vector<TelegramItem> TelegramRepository::get(player_t player,
                                                  governor_t governor) {
  return find_where("recipient_player = ? AND recipient_governor = ? "
                    "ORDER BY timestamp ASC, id ASC",
                    {player, governor});
}

void TelegramRepository::delete_for_governor(player_t player,
                                             governor_t governor) {
  remove_where("recipient_player = ? AND recipient_governor = ?",
               {player, governor});
}

int TelegramRepository::count(player_t player, governor_t governor) {
  return count_where("recipient_player = ? AND recipient_governor = ?",
                     {player, governor});
}

bool TelegramRepository::purge_all() {
  return remove_where();
}
