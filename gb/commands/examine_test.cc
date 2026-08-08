// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Initialize universe
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player race
  Race race{};
  race.Playernum = 1;
  race.name = "Inspectors";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Initialize star
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.inhabited = (1ULL << 1);  // Player 1 inhabits
  Star star0(ss0);
  StarRepository stars(store);
  stars.save(star0);

  // Create a ship owned by player 1 at star 0
  ship_struct sdata{};
  sdata.number = 1;
  sdata.owner = 1;
  sdata.type = ShipType::STYPE_SHUTTLE;
  sdata.whatorbits = ScopeLevel::LEVEL_STAR;
  sdata.storbits = 0;
  sdata.alive = 1;
  sdata.active = 1;

  Ship ship1(sdata);
  ShipRepository ships(store);
  ships.save(ship1);

  // Seed / set a custom ShipExam description in SQLite
  auto exam_handle = ctx.em.get_ship_exam(ShipType::STYPE_SHUTTLE);
  exam_handle->description =
      "Shuttle: SQLite stored short-range spacecraft description.";

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // Execute examine #1
  command_t argv = {"examine", "#1"};
  GB::commands::examine(argv, g);

  std::string output = g.out.str();
  assert(output.find(
             "Shuttle: SQLite stored short-range spacecraft description.") !=
         std::string::npos);

  std::println(std::cout, "✓ examine_test passed successfully!");
  return 0;
}
