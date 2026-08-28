// SPDX-License-Identifier: Apache-2.0

/// \file auth.cppm
/// \brief Authentication and connection handshake service.

module;

import std;

export module auth;

import commands;
import dallib;
import gblib;
import session;

export struct ConnectionPassword {
  std::string player;
  std::string governor;
};

export command_t make_command_t(std::string_view message);
export ConnectionPassword parse_connect(std::string_view message);
export void welcome_user(Session& session, EntityManager& entity_manager);
export void check_connect(Session& session, std::string_view message);
