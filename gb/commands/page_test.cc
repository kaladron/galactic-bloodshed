// SPDX-License-Identifier: Apache-2.0

/// \file page_test.cc
/// \brief Test page command functionality, alliance block paging, and scope
/// validations.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_page_dispatch() {
  std::println(std::cout, "Test: page command dispatch and notifications");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Klingons";
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup block for player 1
  block block1{};
  block1.Playernum = 1;
  block1.name = "AlphaAlliance";
  block1.invite = 0b11;
  block1.pledge = 0b11;
  BlockRepository blocks(store);
  blocks.save(block1);

  // Create star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;
  star_data.name = "Sol";
  Star star{star_data};
  star.AP(player_t{1}) = 10;
  StarRepository stars(store);
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Page player 2
  ctx.assert_dispatch_success(g, {"page", "2"});
  assert(g.out.str().contains("Request sent."));
  std::println(std::cout, "    ✓ Paged player 2 successfully");

  // 2. Page alliance block
  g.out.str("");
  ctx.assert_dispatch_success(g, {"page", "block"});
  assert(g.out.str().contains("Request sent."));
  std::println(std::cout, "    ✓ Paged alliance block successfully");

  // 3. Scope check: page at universal scope rejected
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "2"});
  assert(g.out.str().contains("Invalid scope for this command."));
  std::println(std::cout, "    ✓ Universal scope rejection verified for page");

  // 4. Invalid target player rejection
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "99"});
  assert(g.out.str().contains("No such player."));
  std::println(std::cout, "    ✓ Invalid player rejection verified");
}

}  // namespace

int main() {
  test_page_dispatch();
  std::println(std::cout, "\n✅ All page tests passed!");
  return 0;
}
