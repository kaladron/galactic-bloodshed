// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

bool mock_success_cmd(const command_t&, GameObj& g) {
  g.out << "mock success\n";
  return true;
}

bool mock_failure_cmd(const command_t&, GameObj& g) {
  g.out << "mock failure\n";
  return false;
}

void test_test_context_dispatch_helpers() {
  TestContext ctx;

  // Setup Star 1 with 20 AP
  {
    JsonStore store(ctx.db);
    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 1;
    sdata.AP[0] = 20;
    Star star{sdata};
    star_repo.save(star);
  }

  // Setup Universe with 30 AP
  {
    JsonStore store(ctx.db);
    UniverseRepository universe_repo(store);
    universe_struct u{};
    u.id = 1;
    u.AP[0] = 30;
    universe_repo.save(u);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_snum(1);

  GB::commands::CommandDescriptor star_cost_cmd{
      .name = "star_cost",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(5),
      .handler = &mock_success_cmd,
  };

  GB::commands::CommandDescriptor univ_cost_cmd{
      .name = "univ_cost",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_univ(10),
      .handler = &mock_success_cmd,
  };

  GB::commands::CommandDescriptor fail_cmd{
      .name = "fail_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(5),
      .handler = &mock_failure_cmd,
  };

  // 1. Success dispatch with star AP verification (20 -> 15)
  ctx.assert_dispatch_success(g, star_cost_cmd, {"star_cost"},
                              /*expected_star_ap_deducted=*/5);
  assert(ctx.em.peek_star(1)->AP(player_t{1}) == 15);

  // 2. Success dispatch with universe AP verification (30 -> 20)
  ctx.assert_dispatch_success(g, univ_cost_cmd, {"univ_cost"},
                              /*expected_star_ap_deducted=*/0,
                              /*expected_univ_ap_deducted=*/10);
  assert(ctx.em.peek_universe()->AP[0] == 20);

  // 3. Rejected dispatch due to handler returning false (AP remains 15)
  ctx.assert_dispatch_rejected(g, fail_cmd, {"fail_cmd"});
  assert(ctx.em.peek_star(1)->AP(player_t{1}) == 15);

  // 4. Rejected dispatch due to insufficient AP (have 15, need 50 -> remains
  // 15)
  GB::commands::CommandDescriptor expensive_cmd{
      .name = "expensive_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(50),
      .handler = &mock_success_cmd,
  };
  ctx.assert_dispatch_rejected(g, expensive_cmd, {"expensive_cmd"});
  assert(ctx.em.peek_star(1)->AP(player_t{1}) == 15);
}

}  // namespace

int main() {
  test_test_context_dispatch_helpers();
  std::println(std::cout, "✓ test_context_test passed!");
  return 0;
}
