// SPDX-License-Identifier: Apache-2.0

/// \file auth.cc
/// \brief Implementation of authentication, password parsing, and login
/// handshake.

module;

import commands;
import dallib;
import gb.entities;
import gb.presentation;
import gb.services;
import gb.turn;
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

  if (argv.size() == 2) {
    return {argv[0], argv[1], UiMode::ASCII};
  }
  if (argv.size() == 3 && (argv[2] == "json" || argv[2] == "JSON")) {
    return {argv[0], argv[1], UiMode::JSON};
  }

  return {"", "", UiMode::ASCII};
}

void welcome_user(Session& session, EntityManager& entity_manager) {
  session.send(std::format("***   Welcome to Galactic Bloodshed v{} ***\n"
                           "Please enter your password:\n",
                           GB_VERSION));

  const auto* state = entity_manager.peek_server_state();
  if (state && !state->welcome_message.empty()) {
    session.send(state->welcome_message);
    if (!state->welcome_message.ends_with('\n')) {
      session.send("\n");
    }
  }

  // Immediately flush welcome message (before command loop starts)
  session.flush_to_network();
}

void check_connect(Session& session, std::string_view message) {
  auto [race_password, gov_password, requested_mode] = parse_connect(message);
  if (requested_mode == UiMode::JSON) {
    session.set_ui_mode(UiMode::JSON);
  }

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
    session.send("Connection refused.\n");
    std::println(std::cerr, "FAILED CONNECT {},{}", race_password,
                 gov_password);
    return;
  }

  bool authenticated = false;
  GB::presentation::LoginViewModel login_vm{};
  try {
    session.entity_manager().with_race(Playernum, [&](const Race& race) {
      // Check if player is already connected
      if (session.registry().is_connected(Playernum, Governor)) {
        session.send("Connection refused.\n");
        return;
      }
      authenticated = true;

      const auto& gov = race.governor(Governor);
      std::println(std::cerr, "CONNECTED {} \"{}\" [{},{}]", race.name,
                   gov.name, Playernum, Governor);
      session.set_connected(true);
      session.set_god(race.God);
      session.set_player(Playernum);
      session.set_governor(Governor);

      // Initialize scope to default or safe values
      session.set_level(gov.deflevel);
      session.set_snum(gov.defsystem);
      session.set_pnum(gov.defplanetnum);
      session.set_shipno(0);

      // Validate and clamp star number
      const starnum_t numstars = session.entity_manager().num_stars();
      if (session.snum() < 1 || session.snum() > numstars) {
        session.set_snum(1);  // Default to first star if invalid
      }

      // Validate and clamp planet number
      session.entity_manager().with_star(
          session.snum(), [&](const Star& init_star) {
            if (session.pnum() < 1 || session.pnum() > init_star.numplanets()) {
              session.set_pnum(1);  // Default to first planet if invalid
            }
          });

      login_vm = GB::presentation::LoginViewModel{
          .race_id = Playernum,
          .gov_id = Governor,
          .race_name = race.name,
          .gov_name = gov.name,
          .invisible = gov.toggle.invisible,
          .last_login = format_timestamp(gov.login),
          .gov_ship = race.Gov_ship,
          .morale = race.morale,
      };
    });
  } catch (const EntityNotFoundError&) {
    session.send("Connection refused.\n");
    return;
  }
  if (!authenticated) return;

  auto& g = session.game_obj();
  if (session.ui_mode() == UiMode::JSON) {
    session.send(GB::presentation::render_json_login(login_vm));
  } else {
    session.send(GB::presentation::render_login_banner(login_vm));
  }

  // Display time and treasury via centralized command dispatch pipeline
  g.out.str("");
  g.out.clear();
  GB::commands::dispatch_command(g, {"time"});
  session.send(g.out.view());
  g.out.str("");
  g.out.clear();

  if (session.ui_mode() != UiMode::JSON) {
    session.send(GB::presentation::render_login_status(login_vm));
  }

  GB::commands::dispatch_command(g, {"treasury"});
  session.send(g.out.view());
  g.out.str("");
  g.out.clear();
  g.race = nullptr;

  // Update login time
  session.entity_manager().mutate_race(Playernum, [&](Race& race_mut) {
    race_mut.governor(Governor).login = std::time(nullptr);
  });
}
