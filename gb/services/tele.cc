// SPDX-License-Identifier: Apache-2.0

/// \file tele.cc
/// \brief Telegram functions

module;

import std;

module gb.services;

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
  const auto& race = *em.peek_race(recipient);

  for (auto [j, gov] : race.active_governors())
    push_telegram(em, recipient, j, msg);
}

void telegram_star(EntityManager& em, starnum_t star, player_t sender,
                   governor_t sender_gov, const std::string& message) {
  const auto& star_ref = *em.peek_star(star);

  for (const Race& race : RaceList::readonly(em)) {
    const player_t p = race.Playernum;
    if (!star_ref.is_inhabited_by(p)) continue;

    for (auto [i, gov] : race.active_governors()) {
      if (p == sender && i == sender_gov) continue;
      push_telegram(em, p, i, message);
    }
  }
}

std::vector<TelegramItem> teleg_read(EntityManager& em, player_t player,
                                     governor_t governor) {
  auto telegrams = em.get_telegrams(player, governor);
  if (!telegrams.empty()) {
    em.delete_telegrams(player, governor);
  }
  return telegrams;
}

std::vector<NewsItem> news_read(EntityManager& em, player_t player,
                                governor_t governor, NewsType type) {
  const auto last_read_id =
      em.peek_race(player)->governor(governor).newspos[type];
  auto news_items = em.get_news_since(type, last_read_id);
  if (news_items.empty()) {
    return news_items;
  }

  const auto latest_id = std::ranges::max(news_items, {}, &NewsItem::id).id;
  em.mutate_race(player, [&](Race& race) {
    race.governor(governor).newspos[type] = latest_id;
  });
  return news_items;
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
