// SPDX-License-Identifier: Apache-2.0

// \file power.c display power report

module;

import gblib;
import std;

module commands;

namespace {
std::string prepare_output_line(EntityManager& em, const Race& race,
                                const Race& r, player_t i, int rank) {
  std::stringstream ss;
  if (rank != 0) ss << std::format("{:2d} ", rank);

  ss << std::format("[{:2d}]{}{}{:<15.15s} {:5s}", i,
                    isset(race.allied, i) ? "+"
                                          : (isset(race.atwar, i) ? "-" : " "),
                    isset(r.allied, race.Playernum)
                        ? "+"
                        : (isset(r.atwar, race.Playernum) ? "-" : " "),
                    r.name, Estimate_i((int)r.victory_score, race, i));
  const auto* power_ptr = em.peek_power(i);
  if (!power_ptr) return "";
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->troops, race, i));
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->popn, race, i));
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->money, race, i));
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->ships_owned, race, i));
  ss << std::format("{:3s}",
                    Estimate_i((int)power_ptr->planets_owned, race, i));
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->resource, race, i));
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->fuel, race, i));
  ss << std::format("{:5s}", Estimate_i((int)power_ptr->destruct, race, i));
  ss << std::format("{:5s}", Estimate_i((int)r.morale, race, i));
  if (race.God) {
    const auto* universe = em.peek_universe();
    ss << std::format(" {:3d}\n", universe->VN_hitlist[i - 1]);
  } else
    ss << std::format(" {:3d}%\n", race.translate[i - 1]);

  return ss.str();
}
}  // namespace

namespace GB::commands {
void power(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  // TODO(jeffbailey): ap_t APcount = 0;
  // TODO(jeffbailey): Need to stop using -1 here for UB
  player_t p = -1;

  if (argv.size() >= 2) {
    if (!(p = get_player(g.entity_manager, argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
  }

  const auto* race = g.race;  // Use pre-populated race from GameObj

  g.out << std::format(
      "         ========== Galactic Bloodshed Power Report ==========\n");

  if (g.race->God)
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
    auto vicvec = create_victory_list(g.entity_manager);
    int rank = 0;
    for (const auto& vic : vicvec) {
      rank++;
      p = vic.racenum;
      const auto* r = g.entity_manager.peek_race(p);
      if (!r) continue;
      if (!r->dissolved && race->translate[p - 1] >= 10) {
        g.out << prepare_output_line(g.entity_manager, *race, *r, p, rank);
      }
    }
  } else {
    const auto* r = g.entity_manager.peek_race(p);
    if (!r) {
      g.out << "Race not found.\n";
      return;
    }
    g.out << prepare_output_line(g.entity_manager, *race, *r, p, 0);
  }
}
}  // namespace GB::commands
