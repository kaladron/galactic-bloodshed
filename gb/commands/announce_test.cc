// SPDX-License-Identifier: Apache-2.0

/// \file announce_test.cc
/// \brief Test announce, broadcast, shout, and think communication commands and
/// role checks.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_announce_dispatch() {
  std::println(std::cout,
               "Test: announce, broadcast, shout, and think dispatch");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test race
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;
  race1.governor[0].name = "President";

  RaceRepository races(store);
  races.save(race1);

  // Create star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;
  star_data.name = "Sol";
  Star star{star_data};
  setbit<std::uint64_t>(star.inhabited(), 1U);
  StarRepository stars(store);
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Announce in inhabited star system
  ctx.assert_dispatch_success(g, {"announce", "Hello", "System"});
  std::println(std::cout, "    ✓ Star system announcement succeeded");

  // 2. Broadcast across galaxy
  ctx.assert_dispatch_success(g, {"broadcast", "Global", "Transmission"});
  std::println(std::cout, "    ✓ Broadcast transmission succeeded");

  // 3. Broadcast alias "'"
  ctx.assert_dispatch_success(g, {"'", "Quick", "Message"});
  std::println(std::cout, "    ✓ Apostrophe alias for broadcast succeeded");

  // 4. Think to race governors
  ctx.assert_dispatch_success(g, {"think", "Internal", "Memo"});
  std::println(std::cout, "    ✓ Think command succeeded");

  // 5. Shout rejected for mortal
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"shout", "Deity", "Announcement"});
  assert(g.out.str().contains("Only deity can use this command."));
  std::println(std::cout, "    ✓ Shout rejection for mortal verified");

  // 6. Shout succeeds for deity
  g.set_god(true);
  ctx.assert_dispatch_success(g, {"shout", "Deity", "Announcement"});
  std::println(std::cout, "    ✓ Shout succeeded for deity");
}

}  // namespace

int main() {
  test_announce_dispatch();
  std::println(std::cout, "\n✅ All announce tests passed!");
  return 0;
}
