// SPDX-License-Identifier: Apache-2.0

/// \file test_context_test.cc
/// \brief Unit tests for TestContext and test expectation assertion utilities.

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

bool mock_success_cmd(const command_t&, GameObj& g) {
  g.out << "mock success\n";
  return true;
}

bool mock_failure_cmd(const command_t&, GameObj& g) {
  g.out << "mock failure\n";
  return false;
}

void test_expectation_utilities() {
  std::println(std::cout,
               "Test: Modern expectation and diagnostic assertion utilities");

  // 1. Basic equality & inequality
  test::expect_eq(42, 42, "Integer equality");
  test::expect_eq(std::string("hello"), std::string("hello"),
                  "String equality");
  test::expect_ne(10, 20, "Integer inequality");

  // 2. Strong ID type comparisons (formats seamlessly)
  player_t p1{1};
  player_t p2{1};
  player_t p3{2};
  test::expect_eq(p1, p2, "Strong ID equality");
  test::expect_ne(p1, p3, "Strong ID inequality");

  starnum_t s1{0};
  test::expect_eq(s1, starnum_t{0}, "Starnum equality");

  // 3. Relational comparisons (ge, le, gt, lt)
  test::expect_ge(10, 10, "Greater or equal (equal case)");
  test::expect_ge(15, 10, "Greater or equal (greater case)");
  test::expect_le(10, 10, "Less or equal (equal case)");
  test::expect_le(5, 10, "Less or equal (less case)");
  test::expect_gt(15, 10, "Strictly greater");
  test::expect_lt(5, 10, "Strictly less");

  // 4. Boolean assertions
  test::expect_true(true, "True condition");
  test::expect_true(1 + 1 == 2, "Expression true");
  test::expect_false(false, "False condition");
  test::expect_false(1 + 1 == 3, "Expression false");

  // 5. String contains
  std::string output = "Sector 5,5 colonized by player 1 (Federation)";
  test::expect_contains(output, "colonized by player 1", "Sub-string matching");
  test::expect_contains(output, "Federation");

  // 6. Exception expectations
  test::expect_throws<std::runtime_error>(
      []() { throw std::runtime_error("Simulated domain error"); },
      "Expected std::runtime_error");

  test::expect_throws<EntityNotFoundError>(
      []() { throw EntityNotFoundError("Race not found: player=999"); },
      "Expected EntityNotFoundError");

  test::expect_no_throw(
      []() {
        int x = 10 + 20;
        (void)x;
      },
      "Expected safe lambda without exception");

  std::println(std::cout,
               "  ✓ All expectation utilities verified successfully");
}

void test_test_context_dispatch_helpers() {
  std::println(std::cout,
               "Test: TestContext dispatch and AP accounting helpers");
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
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);
  test::expect_contains(g.out.str(), "mock success");

  // 2. Success dispatch with universe AP verification (30 -> 20)
  ctx.assert_dispatch_success(g, univ_cost_cmd, {"univ_cost"},
                              /*expected_star_ap_deducted=*/0,
                              /*expected_univ_ap_deducted=*/10);
  test::expect_eq(ctx.em.peek_universe()->AP[0], 20);

  // 3. Rejected dispatch due to handler returning false (AP remains 15)
  ctx.assert_dispatch_rejected(g, fail_cmd, {"fail_cmd"});
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);

  // 4. Rejected dispatch due to insufficient AP (have 15, need 50 -> remains
  // 15)
  GB::commands::CommandDescriptor expensive_cmd{
      .name = "expensive_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(50),
      .handler = &mock_success_cmd,
  };
  ctx.assert_dispatch_rejected(g, expensive_cmd, {"expensive_cmd"});
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);
}

}  // namespace

int main() {
  test_expectation_utilities();
  test_test_context_dispatch_helpers();
  std::println(std::cout, "✓ test_context_test passed!");
  return 0;
}
