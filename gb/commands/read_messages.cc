// SPDX-License-Identifier: Apache-2.0

/// \file read_messages.cc
/// \brief Read telegrams, news bulletins, and announcements.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool read_messages(const command_t& argv, GameObj& g) {
  if (argv.size() > 2) {
    g.out << "Read what?\n";
    return false;
  }
  if (argv.size() == 1 || argv[1] == "telegram") {
    g.present(GB::presentation::make_telegram_read_result(
        teleg_read(g.entity_manager, g.player(), g.governor())));
    return true;
  }
  if (argv[1] == "news") {
    auto& em = g.entity_manager;
    const auto player = g.player();
    const auto governor = g.governor();
    g.present(GB::presentation::make_news_bundle_result(
        news_read(em, player, governor, NewsType::DECLARATION),
        news_read(em, player, governor, NewsType::COMBAT),
        news_read(em, player, governor, NewsType::TRANSFER),
        news_read(em, player, governor, NewsType::ANNOUNCE)));
    return true;
  }
  g.out << "Read what?\n";
  return false;
}

const CommandDescriptor read_cmd{
    .name = "read",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "read [telegram|news]",
    .description = "Read telegram messages or public news bulletins",
    .handler = &read_messages,
};

}  // namespace GB::commands
