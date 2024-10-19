// SPDX-License-Identifier: Apache-2.0

// \file power.c display power report

module;

import gblib;
import std;

module commands;

namespace {
std::string prepare_output_line(const Race &race, const Race &r, player_t i,
                                int rank) {
  std::stringstream ss;
  if (rank != 0) ss << std::format("{:2d} ", rank);

  ss << std::format(
      "[{:2d}]{}{}{:<15.15s} {:5s}", i,
      isset(race.allied, i) ? "+" : (isset(race.atwar, i) ? "-" : " "),
      isset(r.allied, race.Playernum)
          ? "+"
          : (isset(r.atwar, race.Playernum) ? "-" : " "),
      r.name, Estimate_i((int)r.victory_score, race, i));
  ss << std::format("{:5s}", Estimate_i((int)Power[i - 1].troops, race, i));
  ss << std::format("{:5s}", Estimate_i((int)Power[i - 1].popn, race, i));
  ss << std::format("{:5s}", Estimate_i((int)Power[i - 1].money, race, i));
  ss << std::format("{:5s}",
                    Estimate_i((int)Power[i - 1].ships_owned, race, i));
  ss << std::format("{:3s}",
                    Estimate_i((int)Power[i - 1].planets_owned, race, i));
  ss << std::format("{:5s}", Estimate_i((int)Power[i - 1].resource, race, i));
  ss << std::format("{:5s}", Estimate_i((int)Power[i - 1].fuel, race, i));
  ss << std::format("{:5s}", Estimate_i((int)Power[i - 1].destruct, race, i));
  ss << std::format("{:5s}", Estimate_i((int)r.morale, race, i));
  if (race.God)
    ss << std::format(" {:3d}\n", Sdata.VN_hitlist[i - 1]);
  else
    ss << std::format(" {:3d}%\n", race.translate[i - 1]);

  return ss.str();
}
}  // namespace

namespace GB::commands {
void power(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  // TODO(jeffbailey): ap_t APcount = 0;
  // TODO(jeffbailey): Need to stop using -1 here for UB
  player_t p = -1;

  if (argv.size() >= 2) {
    if (!(p = get_player(argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
  }

  auto &race = races[Playernum - 1];

  g.out << std::format(
      "         ========== Galactic Bloodshed Power Report ==========\n");

  if (race.God)
    g.out << std::format(
        "{}  #  Name               VP  mil  civ cash ship pl  res fuel dest "
        "morl VNs\n",
        argv.size() < 2 ? "rank" : "");
  else
    g.out << std::format(
        "{}  #  Name               VP  mil  civ cash ship pl  res fuel dest "
        "morl know\n",
        argv.size() < 2 ? "rank" : "");

  if (argv.size() < 2) {
    auto vicvec = create_victory_list();
    int rank = 0;
    for (const auto &vic : vicvec) {
      rank++;
      p = vic.racenum;
      auto &r = races[p - 1];
      if (!r.dissolved && race.translate[p - 1] >= 10) {
        g.out << prepare_output_line(race, r, p, rank);
      }
    }
  } else {
    auto &r = races[p - 1];
    g.out << prepare_output_line(race, r, p, 0);
  }
}
}  // namespace GB::commands
