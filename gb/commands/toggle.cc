// SPDX-License-Identifier: Apache-2.0

/// \file toggle.cc
/// \brief toggles some options

module;

import gblib;
import std;

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
void toggle(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  // TODO(jeffbailey): ap_t APcount = 0;

  auto race_handle = g.entity_manager.get_race(Playernum);

  if (argv.size() == 1) {
    const auto& race = race_handle.read();
    display_toggles(g, race.governor[Governor], race);
    return;
  }

  auto& race = *race_handle;

  if (argv[1] == "inverse")
    tog(g, &race.governor[Governor].toggle.inverse, "inverse");
  else if (argv[1] == "double_digits")
    tog(g, &race.governor[Governor].toggle.double_digits, "double_digits");
  else if (argv[1] == "geography")
    tog(g, &race.governor[Governor].toggle.geography, "geography");
  else if (argv[1] == "gag")
    tog(g, &race.governor[Governor].toggle.gag, "gag");
  else if (argv[1] == "autoload")
    tog(g, &race.governor[Governor].toggle.autoload, "autoload");
  else if (argv[1] == "color")
    tog(g, &race.governor[Governor].toggle.color, "color");
  else if (argv[1] == "visible")
    tog(g, &race.governor[Governor].toggle.invisible, "invisible");
  else if (race.God && argv[1] == "monitor")
    tog(g, &race.monitor, "monitor");
  else if (argv[1] == "compatibility")
    tog(g, &race.governor[Governor].toggle.compat, "compatibility");
  else {
    g.out << std::format("No such option '{}'\n", argv[1]);
    return;
  }
}
}  // namespace GB::commands