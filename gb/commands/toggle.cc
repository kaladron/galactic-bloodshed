// SPDX-License-Identifier: Apache-2.0

/// \file toggle.cc
/// \brief toggles some options

module;

import gblib;
import std;
#undef stdout

module commands;

namespace {
void tog(GameObj& g, bool* op, const char* name) {
  *op = !(*op);
  g.out << std::format("{0} is now {1}\n", name, *op ? "on" : "off");
}

void display_toggles(GameObj& g, const Race::gov& governor, const Race& race) {
  g.out << std::format("gag is {}\n", governor.toggle.gag ? "ON" : "OFF");
  g.out << std::format("inverse is {}\n",
                       governor.toggle.inverse ? "ON" : "OFF");
  g.out << std::format("double_digits is {}\n",
                       governor.toggle.double_digits ? "ON" : "OFF");
  g.out << std::format("geography is {}\n",
                       governor.toggle.geography ? "ON" : "OFF");
  g.out << std::format("autoload is {}\n",
                       governor.toggle.autoload ? "ON" : "OFF");
  g.out << std::format("color is {}\n", governor.toggle.color ? "ON" : "OFF");
  g.out << std::format("compatibility is {}\n",
                       governor.toggle.compat ? "ON" : "OFF");
  g.out << std::format("{}\n",
                       governor.toggle.invisible ? "INVISIBLE" : "VISIBLE");
  g.out << std::format("highlight player {}\n", governor.toggle.highlight);
  if (race.God) {
    g.out << std::format("monitor is {}\n", race.monitor ? "ON" : "OFF");
  }
}
}  // namespace

namespace GB::commands {

bool toggle(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (argv.size() == 1) {
    g.entity_manager.with_race(Playernum, [&](const Race& race) {
      display_toggles(g, race.governor[Governor.value], race);
    });
    return true;
  }

  bool result = false;
  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    if (argv[1] == "inverse") {
      tog(g, &race.governor[Governor.value].toggle.inverse, "inverse");
      result = true;
    } else if (argv[1] == "double_digits") {
      tog(g, &race.governor[Governor.value].toggle.double_digits,
          "double_digits");
      result = true;
    } else if (argv[1] == "geography") {
      tog(g, &race.governor[Governor.value].toggle.geography, "geography");
      result = true;
    } else if (argv[1] == "gag") {
      tog(g, &race.governor[Governor.value].toggle.gag, "gag");
      result = true;
    } else if (argv[1] == "autoload") {
      tog(g, &race.governor[Governor.value].toggle.autoload, "autoload");
      result = true;
    } else if (argv[1] == "color") {
      tog(g, &race.governor[Governor.value].toggle.color, "color");
      result = true;
    } else if (argv[1] == "visible") {
      tog(g, &race.governor[Governor.value].toggle.invisible, "invisible");
      result = true;
    } else if (race.God && argv[1] == "monitor") {
      tog(g, &race.monitor, "monitor");
      result = true;
    } else if (argv[1] == "compatibility") {
      tog(g, &race.governor[Governor.value].toggle.compat, "compatibility");
      result = true;
    } else {
      g.out << std::format("No such option '{}'\n", argv[1]);
      result = false;
    }
  });
  return result;
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