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

class CapturingSessionRegistry : public NullSessionRegistry {
public:
  std::vector<std::string> messages;

  bool notify_player(player_t, governor_t, const std::string& msg) override {
    messages.push_back(msg);
    return true;
  }
};

void test_announce_dispatch() {
  std::println(std::cout,
               "Test: announce, broadcast, shout, and think dispatch");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test race 1
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;
  race1.governor[0].name = "President";
  race1.governor[1].active = true;
  race1.governor[1].name = "VicePresident";

  // Setup test race 2 (to receive broadcasts/announcements)
  Race race2{};
  race2.Playernum = 2;
  race2.name = "Empire";
  race2.governor[0].active = true;
  race2.governor[0].name = "Emperor";

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Create star inhabited by both races
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;
  star_data.name = "Sol";
  Star star{star_data};
  setbit<std::uint64_t>(star.inhabited(), 1U);
  setbit<std::uint64_t>(star.inhabited(), 2U);
  StarRepository stars(store);
  stars.save(star);

  CapturingSessionRegistry registry;
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Announce in inhabited star system (separator ':')
  registry.messages.clear();
  ctx.assert_dispatch_success(g, {"announce", "Hello", "System"});
  assert(!registry.messages.empty());
  assert(registry.messages.back().contains(": Hello System"));
  std::println(
      std::cout,
      "    ✓ Star system announcement succeeded with canonical ':' separator");

  // 2. Broadcast across galaxy (separator '>')
  registry.messages.clear();
  ctx.assert_dispatch_success(g, {"broadcast", "Global", "Transmission"});
  assert(!registry.messages.empty());
  assert(registry.messages.back().contains("> Global Transmission"));
  std::println(
      std::cout,
      "    ✓ Broadcast transmission succeeded with canonical '>' separator");

  // 3. Broadcast alias "'" (separator '>')
  registry.messages.clear();
  ctx.assert_dispatch_success(g, {"'", "Quick", "Message"});
  assert(!registry.messages.empty());
  assert(registry.messages.back().contains("> Quick Message"));
  std::println(std::cout, "    ✓ Apostrophe alias for broadcast succeeded with "
                          "canonical '>' separator");

  // 4. Think to race governors (separator '=')
  registry.messages.clear();
  ctx.assert_dispatch_success(g, {"think", "Internal", "Memo"});
  assert(!registry.messages.empty());
  assert(registry.messages.back().contains("= Internal Memo"));
  std::println(std::cout,
               "    ✓ Think command succeeded with canonical '=' separator");

  // 5. Shout rejected for mortal
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"shout", "Deity", "Announcement"});
  assert(g.out.str().contains("Only deity can use this command."));
  std::println(std::cout, "    ✓ Shout rejection for mortal verified");

  // 6. Shout succeeds for deity (separator '!')
  registry.messages.clear();
  g.set_god(true);
  ctx.assert_dispatch_success(g, {"shout", "Deity", "Announcement"});
  assert(!registry.messages.empty());
  assert(registry.messages.back().contains("! Deity Announcement"));
  std::println(std::cout,
               "    ✓ Shout succeeded for deity with canonical '!' separator");
}

}  // namespace

int main() {
  test_announce_dispatch();
  std::println(std::cout, "\n✅ All announce tests passed!");
  return 0;
}
