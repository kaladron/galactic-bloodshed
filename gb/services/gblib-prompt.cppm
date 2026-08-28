// SPDX-License-Identifier: Apache-2.0

/// \file gblib-prompt.cppm
/// \brief Module partition for player command prompt generation.

module;

import std;

export module gblib:prompt;

import :gameobj;

export std::string do_prompt(const GameObj& g);
