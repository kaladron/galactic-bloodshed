// SPDX-License-Identifier: Apache-2.0

/// \file auth.cc
/// \brief Implementation of authentication, password parsing, and login
/// handshake.

module;

#include <cstdio>

import commands;
import dallib;
import gb.entities;
import gb.services;
import session;
import std;

module auth;

command_t make_command_t(std::string_view message) {
  command_t argv;

  std::size_t position;
  while ((position = message.find(' ')) != std::string_view::npos) {
    if (position == 0) {
      message.remove_prefix(1);
      continue;
    }
    argv.emplace_back(message.substr(0, position));
    message.remove_prefix(position + 1);
  }

  if (!message.empty()) argv.emplace_back(message);

  return argv;
}

/**
 * \brief Parse input string for player and governor password
 * \param message Input string from the user
 * \return player and governor password or empty strings if invalid
 */
ConnectionPassword parse_connect(const std::string_view message) {
  auto argv = make_command_t(message);

  if (argv.size() != 2) {
    return {"", ""};
  }

  return {argv[0], argv[1]};
}

void welcome_user(Session& session, EntityManager& entity_manager) {
  session.out() << std::format("***   Welcome to Galactic Bloodshed {} ***\n"
                               "Please enter your password:\n",
                               GB_VERSION);

  const auto* state = entity_manager.peek_server_state();
  if (state && !state->welcome_message.empty()) {
    session.out() << state->welcome_message;
    if (!state->welcome_message.ends_with('\n')) {
      session.out() << "\n";
    }
  }

  // Immediately flush welcome message (before command loop starts)
  session.flush_to_network();
}

void check_connect(Session& session, std::string_view message) {
  auto [race_password, gov_password] = parse_connect(message);

  if (EXTERNAL_TRIGGER) {
    if (race_password == SEGMENT_PASSWORD) {
      do_segment(session.entity_manager(), session.registry(), 1, 0);
      return;
    } else if (race_password == UPDATE_PASSWORD) {
      do_update(session.entity_manager(), session.registry(), true);
      return;
    }
  }

  auto [Playernum, Governor] =
      getracenum(session.entity_manager(), race_password, gov_password);

  if (Playernum == 0) {
    session.out() << "Connection refused.\n";
    std::println(stderr, "FAILED CONNECT {},{}", race_password, gov_password);
    return;
  }

  bool authenticated = false;
  try {
    session.entity_manager().with_race(Playernum, [&](const Race& race) {
      // Check if player is already connected
      if (session.registry().is_connected(Playernum, Governor)) {
        session.out() << "Connection refused.\n";
        return;
      }
      authenticated = true;

      std::println(stderr, "CONNECTED {} \"{}\" [{},{}]", race.name,
                   race.governor[Governor.value].name, Playernum, Governor);
      session.set_connected(true);
      session.set_god(race.God);
      session.set_player(Playernum);
      session.set_governor(Governor);

      // Initialize scope to default or safe values
      session.set_level(race.governor[Governor.value].deflevel);
      session.set_snum(race.governor[Governor.value].defsystem);
      session.set_pnum(race.governor[Governor.value].defplanetnum);
      session.set_shipno(0);

      // Validate and clamp star number
      session.entity_manager().with_universe(
          [&](const universe_struct& universe) {
            if (session.snum() >= universe.numstars) {
              session.set_snum(0);  // Default to first star if invalid
            }
          });

      // Validate and clamp planet number
      session.entity_manager().with_star(
          session.snum(), [&](const Star& init_star) {
            if (session.pnum() >= init_star.numplanets()) {
              session.set_pnum(0);  // Default to first planet if invalid
            }
          });

      // Send login messages
      session.out() << std::format(
          "\n{} \"{}\" [{},{}] logged on.\n", race.name,
          race.governor[Governor.value].name, Playernum, Governor);
      session.out() << std::format(
          "You are {}.\n", race.governor[Governor.value].toggle.invisible
                               ? "invisible"
                               : "visible");

      // Display time
      GameObj temp_g(session.entity_manager(), session.registry());
      temp_g.set_player(Playernum);
      temp_g.set_governor(Governor);
      temp_g.race = &race;
      GB::commands::time({}, temp_g);
      session.out() << temp_g.out.str();
      temp_g.out.str("");

      session.out() << std::format(
          "\nLast login      : {}",
          std::ctime(&(race.governor[Governor.value].login)));

      if (race.Gov_ship == 0) {
        session.out()
            << "You have no Governmental Center.  No action points will be "
               "produced\nuntil you build one and designate a capital.\n";
      } else {
        session.out() << std::format("Government Center #{} is active.\n",
                                     race.Gov_ship);
      }
      session.out() << std::format("     Morale: {}\n", race.morale);

      GB::commands::treasury({}, temp_g);

      // Flush temp_g output to session
      session.out() << temp_g.out.str();
    });
  } catch (const EntityNotFoundError&) {
    session.out() << "Connection refused.\n";
    return;
  }
  if (!authenticated) return;

  // Update login time
  session.entity_manager().mutate_race(Playernum, [&](Race& race_mut) {
    race_mut.governor[Governor.value].login = std::time(nullptr);
  });
}
