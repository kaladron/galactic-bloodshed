// SPDX-License-Identifier: Apache-2.0

/// /file tele.cc
/// \brief Telegram functions

module;

import std;

#include "gb/files.h"

module gblib;

namespace {
const char* get_news_file(NewsType type) {
  switch (type) {
    using enum NewsType;
    case DECLARATION:
      return DECLARATIONFL;
    case TRANSFER:
      return TRANSFERFL;
    case COMBAT:
      return COMBATFL;
    case ANNOUNCE:
      return ANNOUNCEFL;
  }
}
}  // namespace

/**
 * \brief Sends a message to everyone from person to person
 *
 * \param recipient The recipient player
 * \param gov The governor of the recipient player
 * \param msg The message to send
 */
void push_telegram(const player_t recipient, const governor_t gov,
                   std::string_view msg) {
  std::string telefl = std::format("{}.{}.{}", TELEGRAMFL, recipient, gov);

  std::ofstream telegram_file(telefl, std::ios::app);
  if (!telegram_file.is_open()) {
    std::cerr << std::format("Failed to open telegram file {} \n", telefl);
    return;
  }
  auto now = std::chrono::system_clock::now();
  auto current_time = std::chrono::system_clock::to_time_t(now);
  auto* current_tm = std::localtime(&current_time);

  telegram_file << std::format("{:02d}/{:02d} {:02d}:{:02d}:{:02d} {}\n",
                               current_tm->tm_mon + 1, current_tm->tm_mday,
                               current_tm->tm_hour, current_tm->tm_min,
                               current_tm->tm_sec, msg);
}

/**
 * \brief Purges the News files.
 */
void purge() {
  std::error_code ec;
  std::filesystem::resize_file(DECLARATIONFL, 0, ec);
  newslength[NewsType::DECLARATION] = 0;

  std::filesystem::resize_file(COMBATFL, 0, ec);
  newslength[NewsType::COMBAT] = 0;

  std::filesystem::resize_file(ANNOUNCEFL, 0, ec);
  newslength[NewsType::ANNOUNCE] = 0;

  std::filesystem::resize_file(TRANSFERFL, 0, ec);
  newslength[NewsType::TRANSFER] = 0;
}

/**
 * \brief Does the actual posting of messages to the news files
 *
 * \param msg Message to send
 * \param type Type of message.  Valid types are DECLARATION, TRANSFER, COMBAT
 * and ANNOUNCE.
 */
void post(std::string msg, NewsType type) {
  // msg is intentionally a copy as we fix it up in here
  const char* telefl = get_news_file(type);

  // look for special symbols
  std::ranges::replace(msg, ';', '\n');
  std::ranges::replace(msg, '|', '\t');

  std::ofstream news_file(telefl, std::ios::app);
  if (!news_file.is_open()) {
    return;
  }
  auto now = std::chrono::system_clock::now();
  auto current_time = std::chrono::system_clock::to_time_t(now);
  auto* current_tm = std::localtime(&current_time);
  std::string outbuf = std::format("{:02d}/{:02d} {:02d}:{:02d}:{:02d} {}",
                                   current_tm->tm_mon + 1, current_tm->tm_mday,
                                   current_tm->tm_hour, current_tm->tm_min,
                                   current_tm->tm_sec, msg);
  news_file << outbuf;
  news_file.close();
  newslength[type] += outbuf.length();
}

/**
 * \brief Sends a message to everyone in the race
 *
 * \param em EntityManager for accessing race data
 * \param recipient Race to receive
 * \param msg Message they will receive
 */
void push_telegram_race(EntityManager& em, const player_t recipient, std::string_view msg) {
  const auto* race = em.peek_race(recipient);
  if (!race) return;
  
  for (governor_t j = 0; j <= MAXGOVERNORS; j++)
    if (race->governor[j].active) push_telegram(recipient, j, msg);
}

/**
 * \brief Read the telegrams for the player.
 *
 * \param g Game object
 *
 * \description The first byte in each telegram is the sending player number or
 * 254 to denote an autoreport. Then the time send, then the message, then
 * terminated by TELEG_DELIM.
 */
void teleg_read(GameObj& g) {
  std::string telegram_file =
      std::format("{0}.{1}.{2}", TELEGRAMFL, g.player, g.governor);

  g.out << "Telegrams:\n";

  std::filesystem::path telegram_path(telegram_file);
  if (!std::filesystem::exists(telegram_path)) {
    g.out << std::format(" None.\n", telegram_file);
    return;
  }

  if (std::filesystem::file_size(telegram_path) == 0) {
    g.out << std::format(" None.\n", telegram_file);
    return;
  }

  std::ifstream teleg_read_fd(telegram_file);
  if (!teleg_read_fd.is_open()) {
    g.out << std::format("Error: Failed to open telegram file {}.\n",
                         telegram_file);
    return;
  }

  std::string line;
  while (std::getline(teleg_read_fd, line)) {
    g.out << line << "\n";
  }

  teleg_read_fd.close();
  std::ofstream truncate_file(telegram_file, std::ios::trunc);
}

/**
 * \brief Read the news file.
 *
 * \param type Type of news. Valid types are DECLARATION, TRANSFER, COMBAT, and
 * ANNOUNCE.
 * \param g Game object
 *
 * \description This function reads the news file based on the specified type
 * and game object.
 */
void news_read(NewsType type, GameObj& g) {
  const char* telegram_file = get_news_file(type);

  std::ifstream teleg_read_fd(telegram_file);
  if (!teleg_read_fd.is_open()) {
    g.out << std::format("\nNews file {0} non-existent.\n", telegram_file);
    return;
  }

  auto race_handle = g.entity_manager.get_race(g.player);
  if (!race_handle.get()) {
    g.out << "Race not found.\n";
    return;
  }
  auto& race = *race_handle;

  if (race.governor[g.governor].newspos[std::to_underlying(type)] >
      newslength[type]) {
    race.governor[g.governor].newspos[std::to_underlying(type)] = 0;
  }

  teleg_read_fd.seekg(
      race.governor[g.governor].newspos[std::to_underlying(type)]);

  std::string line;
  while (std::getline(teleg_read_fd, line)) {
    g.out << line + "\n";
  }

  race.governor[g.governor].newspos[std::to_underlying(type)] =
      newslength[type];
}

/**
 * \brief Check for telegrams and notify the player if there is any.
 *
 * \arg g Game object
 */
void check_for_telegrams(GameObj& g) {
  std::string filename =
      std::format("{}.{}.{}", TELEGRAMFL, g.player, g.governor);

  std::filesystem::path filePath(filename);
  std::error_code ec;
  if (std::filesystem::file_size(filePath, ec) != 0 && !ec) {
    g.out << "You have telegram(s) waiting. Use 'read' to read them.\n";
  }
}
