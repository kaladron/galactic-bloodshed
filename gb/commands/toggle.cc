// SPDX-License-Identifier: Apache-2.0

/// \file toggle.cc
/// \brief toggles some options

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace {

struct ToggleSpec {
  std::string_view arg;
  bool toggletype::* field;
  GB::presentation::ToggleOption option;
};

constexpr std::array<ToggleSpec, 9> toggle_specs{{
    {"inverse", &toggletype::inverse, GB::presentation::ToggleOption::Inverse},
    {"color", &toggletype::color, GB::presentation::ToggleOption::Color},
    {"colour", &toggletype::color, GB::presentation::ToggleOption::Color},
    {"double_digits", &toggletype::double_digits,
     GB::presentation::ToggleOption::DoubleDigits},
    {"geography", &toggletype::geography,
     GB::presentation::ToggleOption::Geography},
    {"gag", &toggletype::gag, GB::presentation::ToggleOption::Gag},
    {"autoload", &toggletype::autoload,
     GB::presentation::ToggleOption::Autoload},
    {"visible", &toggletype::invisible,
     GB::presentation::ToggleOption::Invisible},
    {"compatibility", &toggletype::compat,
     GB::presentation::ToggleOption::Compatibility},
}};

}  // namespace

namespace GB::commands {

bool toggle(const command_t& argv, GameObj& g) {
  if (argv.size() > 2) {
    g.out << "Syntax: toggle [<option>]\n";
    return false;
  }

  if (argv.size() == 1) {
    g.entity_manager.with_race(g.player(), [&](const Race& race) {
      g.present(GB::presentation::ToggleStatusViewModel{
          .toggles = race.governor(g.governor()).toggle,
      });
    });
    return true;
  }

  const auto it = std::ranges::find(toggle_specs, argv[1], &ToggleSpec::arg);
  if (it == toggle_specs.end()) {
    g.out << std::format("No such option '{}'\n", argv[1]);
    return false;
  }

  bool enabled = false;
  g.entity_manager.mutate_race(g.player(), [&](Race& race) {
    auto& field = race.governor(g.governor()).toggle.*(it->field);
    field = !field;
    enabled = field;
  });
  g.present(GB::presentation::ToggleChangedResult{
      .option = it->option,
      .enabled = enabled,
  });
  return true;
}

const CommandDescriptor toggle_cmd{
    .name = "toggle",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "toggle [<option>]",
    .description =
        "Display or toggle client and governor configuration options",
    .handler = &toggle,
};

}  // namespace GB::commands