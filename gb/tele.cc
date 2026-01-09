// SPDX-License-Identifier: Apache-2.0

/// /file tele.cc
/// \brief Telegram functions

module;

import std;

module gblib;

/**
 * \brief Sends a message to everyone from person to person
 *
 * \param em EntityManager for telegram operations
 * \param recipient The recipient player
 * \param gov The governor of the recipient player
 * \param msg The message to send
 */
void push_telegram(EntityManager& em, const player_t recipient,
                   const governor_t gov, std::string_view msg) {
  em.post_telegram(recipient, gov, msg);
}

/**
 * \brief Purges the News files.
 *
 * \param em EntityManager for news operations
 */
void purge(EntityManager& em) {
  em.purge_all_news();
}

/**
 * \brief Does the actual posting of messages to the news database
 *
 * \param em EntityManager for news operations
 * \param msg Message to send
 * \param type Type of message.  Valid types are DECLARATION, TRANSFER, COMBAT
 * and ANNOUNCE.
 */
void post(EntityManager& em, std::string msg, NewsType type) {
  // msg is intentionally a copy as we fix it up in here
  // look for special symbols
  std::ranges::replace(msg, ';', '\n');
  std::ranges::replace(msg, '|', '\t');

  em.post_news(type, msg);
}

/**
 * \brief Sends a message to everyone in the race
 *
 * \param em EntityManager for accessing race data
 * \param recipient Race to receive
 * \param msg Message they will receive
 */
void push_telegram_race(EntityManager& em, const player_t recipient,
                        std::string_view msg) {
  const auto* race = em.peek_race(recipient);
  if (!race) return;

  for (governor_t j = 0; j <= MAXGOVERNORS; ++j)
    if (race->governor[j.value].active) push_telegram(em, recipient, j, msg);
}

/**
 * \brief Read the telegrams for the player.
 *
 * \param g Game object
 *
 * \description Retrieves telegrams from database, displays them, then deletes
 * them (delete-on-read behavior).
 */
void teleg_read(GameObj& g) {
  g.out << "Telegrams:\n";

  // Get all telegrams for this governor
  auto telegrams = g.entity_manager.get_telegrams(g.player(), g.governor());

  if (telegrams.empty()) {
    g.out << " None.\n";
    return;
  }

  // Display telegrams with timestamps
  for (const auto& telegram : telegrams) {
    auto timestamp_time = static_cast<time_t>(telegram.timestamp);
    auto* tm = std::localtime(&timestamp_time);
    g.out << std::format("{:02d}/{:02d} {:02d}:{:02d}:{:02d} {}",
                         tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
                         tm->tm_sec, telegram.message);
    if (!telegram.message.empty() && telegram.message.back() != '\n') {
      g.out << "\n";
    }
  }

  // Delete telegrams after reading (delete-on-read behavior)
  g.entity_manager.delete_telegrams(g.player(), g.governor());
}

/**
 * \brief Read the news file.
 *
 * \param type Type of news. Valid types are DECLARATION, TRANSFER, COMBAT, and
 * ANNOUNCE.
 * \param g Game object
 *
 * \description This function reads the news from database based on the
 * specified type and game object. It tracks which news the user has already
 * read via newspos array in governor data.
 */
void news_read(NewsType type, GameObj& g) {
  auto race_handle = g.entity_manager.get_race(g.player());
  if (!race_handle.get()) {
    g.out << "Race not found.\n";
    return;
  }
  auto& race = *race_handle;

  // Get the last news ID this governor has read for this type
  int last_read_id =
      race.governor[g.governor().value].newspos[std::to_underlying(type)];

  // Get all news since last read
  auto news_items = g.entity_manager.get_news_since(type, last_read_id);

  if (news_items.empty()) {
    // No new news
    return;
  }

  // Display news items with timestamps
  for (const auto& item : news_items) {
    auto timestamp_time = static_cast<time_t>(item.timestamp);
    auto* tm = std::localtime(&timestamp_time);
    g.out << std::format("{:02d}/{:02d} {:02d}:{:02d}:{:02d} {}",
                         tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
                         tm->tm_sec, item.message);
    if (!item.message.empty() && item.message.back() != '\n') {
      g.out << "\n";
    }
  }

  // Update the last read position to the latest ID
  int latest_id = g.entity_manager.get_latest_news_id(type);
  race.governor[g.governor().value].newspos[std::to_underlying(type)] =
      latest_id;
}

/**
 * \brief Check for telegrams and notify the player if there is any.
 *
 * \arg g Game object
 */
void check_for_telegrams(GameObj& g) {
  if (g.entity_manager.has_telegrams(g.player(), g.governor())) {
    g.out << "You have telegram(s) waiting. Use 'read' to read them.\n";
  }
}
