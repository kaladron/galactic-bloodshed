// SPDX-License-Identifier: Apache-2.0

/// \file move_popn_test.cc
/// \brief Unit tests for move and deploy commands

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Set race fighters
  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 10; });

  // Setup sectormap and planet population
  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() = 1000;
    planet.info(player_t{1}).numsectsowned = 2;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{0, 0}).set_owner(0);
    smap.get(Coordinates{0, 0}).set_popn_exact(0);

    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(1000);
    smap.get(Coordinates{5, 5}).set_troops(500);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_MOUNT);

    smap.get(Coordinates{5, 6}).set_owner(1);
    smap.get(Coordinates{5, 6}).set_popn_exact(0);
    smap.get(Coordinates{5, 6}).set_troops(0);
    smap.get(Coordinates{5, 6}).set_condition(SectorType::SEC_LAND);
  });
}

void test_move_popn_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Test move command - 'k' moves south (y+1)
  ctx.assert_dispatch_success(g, {"move", "5,5", "k", "500"});

  // Verify population moved
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(saved_smap != nullptr);

  const auto& source_sect = saved_smap->get(Coordinates{5, 5});
  test::expect_eq(source_sect.get_popn(), 500);

  const auto& dest_sect = saved_smap->get(Coordinates{5, 6});
  test::expect_eq(dest_sect.get_popn(), 500);

  // 2. Test deploy command - deploy 200 troops
  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "200"});

  ctx.em.clear_cache();
  const auto* smap2 = ctx.em.peek_sectormap(1, 1);
  test::expect_true(smap2 != nullptr);
  test::expect_eq(smap2->get(Coordinates{5, 5}).get_troops(), 300);
  test::expect_eq(smap2->get(Coordinates{5, 6}).get_troops(), 200);

  ctx.verify_universe_invariants();
}

void test_move_popn_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_move_popn_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV is not allowed for move/deploy)
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection (Star governed by Gov 1, tested by Gov 2)
  ctx.em.mutate_race(1, [](Race& r) { r.appoint_governor(2); });
  ctx.em.mutate_star(1, [](Star& s) {
    s.governor(1) = 1;  // Player 1, Star governed by Gov 1
  });
  ctx.setup_game_obj(g, 1, 2);  // Player 1, Gov 2
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "not authorized");

  ctx.verify_universe_invariants();
}

void test_move_popn_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"move", "5,5"});
  test::expect_contains(g.out.str(),
                        "Syntax: move <from_sector> <path> [<amount>]");

  // 2. Origin coordinates illegal
  ctx.assert_dispatch_rejected(g, {"move", "99,99", "k"});
  test::expect_contains(g.out.str(), "illegal");

  // 3. Bad value - more people than available in sector
  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "99999"});
  test::expect_contains(g.out.str(), "Bad value");

  // 4. Bad value for military deployment
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"deploy", "5,5", "k", "99999"});
  test::expect_contains(g.out.str(), "troops in");

  // 5. Non-numeric amount string
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "not_a_number"});
  test::expect_contains(g.out.str(), "Bad value");

  // 6. Illegal coordinates (moving north from y=0 off grid edge)
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 0}).set_owner(1);
    smap.get(Coordinates{5, 0}).set_popn_exact(100);
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.popn() += 100;
    p.info(player_t{1}).popn += 100;
    p.info(player_t{1}).numsectsowned += 1;
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "5,0", "j", "10"});
  test::expect_contains(g.out.str(), "Illegal coordinates");

  ctx.verify_universe_invariants();
}

void test_move_popn_assault_and_unowned() {
  TestContext ctx;
  setup_test_world(ctx);

  // Setup sectors: (5,5) owned by P1, (5,6) unowned (owner 0), (5,7) owned by
  // P2
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(1000);
    smap.get(Coordinates{5, 5}).set_troops(500);

    smap.get(Coordinates{5, 6}).set_owner(0);
    smap.get(Coordinates{5, 6}).set_popn_exact(0);
    smap.get(Coordinates{5, 6}).set_troops(0);

    smap.get(Coordinates{5, 7}).set_owner(2);
    smap.get(Coordinates{5, 7}).set_popn_exact(100);
    smap.get(Coordinates{5, 7}).set_troops(50);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() = 1100;
    planet.troops() = 550;
    planet.info(player_t{2}).popn = 100;
    planet.info(player_t{2}).troops = 50;
    planet.info(player_t{2}).numsectsowned = 1;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Move into unowned sector (owner == 0) -> does not trigger assault
  ctx.em.mutate_star(1, [](Star& s) { s.clear_all_ground_assaults(); });
  ctx.assert_dispatch_success(g, {"move", "5,5", "k", "100"});
  test::expect_eq(ctx.em.peek_star(1)->ground_assault_count(1, 2), 0U);

  // 2. Move into enemy sector (owner == 2) -> triggers assault
  ctx.assert_dispatch_success(g, {"move", "5,6", "k", "50"});
  test::expect_ge(ctx.em.peek_star(1)->ground_assault_count(1, 2), 1U);

  // Clean up
  ctx.em.mutate_star(1, [](Star& s) { s.clear_all_ground_assaults(); });
  ctx.verify_universe_invariants();
}

void test_move_popn_negative_and_default_counts() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Move with default count (all 1000 civs)
  ctx.assert_dispatch_success(g, {"move", "5,5", "k"});
  test::expect_contains(g.out.str(), "1000 population moved");

  ctx.em.clear_cache();
  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_eq(smap->get(Coordinates{5, 5}).get_popn(), 0);
  test::expect_eq(smap->get(Coordinates{5, 6}).get_popn(), 1000);

  // 2. Move with negative count: -200 moves all but 200 (1000 - 200 = 800) back
  g.out.str("");
  ctx.assert_dispatch_success(g, {"move", "5,6", "j", "-200"});
  test::expect_contains(g.out.str(), "800 population moved");

  ctx.em.clear_cache();
  smap = ctx.em.peek_sectormap(1, 1);
  test::expect_eq(smap->get(Coordinates{5, 6}).get_popn(), 200);
  test::expect_eq(smap->get(Coordinates{5, 5}).get_popn(), 800);

  // 3. Deploy with default count (all 500 troops)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k"});
  test::expect_contains(g.out.str(), "500 troops moved");

  // 4. Deploy with negative count: -100 moves all but 100 (500 - 100 = 400)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"deploy", "5,6", "j", "-100"});
  test::expect_contains(g.out.str(), "400 troops moved");

  ctx.verify_universe_invariants();
}

void test_move_popn_multistep_path() {
  TestContext ctx;
  setup_test_world(ctx);

  // Setup intermediate sector (5,6) and destination (5,7)
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 6}).set_owner(1);
    smap.get(Coordinates{5, 6}).set_popn_exact(0);

    smap.get(Coordinates{5, 7}).set_owner(1);
    smap.get(Coordinates{5, 7}).set_popn_exact(0);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Multi-step move: 'kk' moves south twice (5,5 -> 5,6 -> 5,7)
  ctx.assert_dispatch_success(g, {"move", "5,5", "kk", "300"});

  ctx.em.clear_cache();
  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_eq(smap->get(Coordinates{5, 5}).get_popn(), 700);
  test::expect_eq(smap->get(Coordinates{5, 7}).get_popn(), 300);

  ctx.verify_universe_invariants();
}

void test_move_popn_enslaved_and_origin_validations() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Enslaved planet check
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.enslave_to(2); });
  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "100"});
  test::expect_contains(g.out.str(), "enslaved");

  // Restore slaved_to
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.free_slaves(); });

  // 2. Bad sector format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "bad_coords", "k", "100"});
  test::expect_contains(g.out.str(), "Bad format for sector");

  // 3. Unowned origin sector
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "0,0", "k", "100"});
  test::expect_contains(g.out.str(), "don't own sector");

  // 4. Unknown direction terminates move
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "5,5", "x", "100"});
  test::expect_contains(g.out.str(), "Finished");

  ctx.verify_universe_invariants();
}

void test_move_popn_assault_metamorph_and_wiped() {
  TestContext ctx;
  setup_test_world(ctx);

  // Configure attacker as Metamorph with absorb enabled
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 50.0;
    r.tech = 100.0;
    r.morale = 150;
    r.absorb = true;
    r.likes[SectorType::SEC_LAND] = 50;
  });

  // Configure defender with minimal defense
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1.0;
    r.tech = 10.0;
    r.morale = 20;
    r.absorb = false;
    r.likes[SectorType::SEC_LAND] = 10;
  });

  // Setup sectors: (5,5) has 1000 civs P1, (5,6) has 10 civs P2
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(1000);
    smap.get(Coordinates{5, 5}).set_troops(0);

    smap.get(Coordinates{5, 6}).set_owner(2);
    smap.get(Coordinates{5, 6}).set_popn_exact(10);
    smap.get(Coordinates{5, 6}).set_troops(0);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() = 1010;
    planet.troops() = 0;
    planet.info(player_t{1}).popn = 1000;
    planet.info(player_t{1}).numsectsowned = 1;
    planet.info(player_t{2}).popn = 10;
    planet.info(player_t{2}).numsectsowned = 1;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Assault and conquer with metamorph attacker
  ctx.assert_dispatch_success(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "VICTORY");

  ctx.em.clear_cache();
  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(smap != nullptr);
  test::expect_eq(smap->get(Coordinates{5, 6}).get_owner(), 1);
  ctx.verify_universe_invariants();

  // Now test defeat where attacker is killed to the last man
  // Setup strong defender at (5,7) with absorb
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 100.0;
    r.tech = 200.0;
    r.morale = 200;
    r.absorb = true;
    r.likes[SectorType::SEC_LAND] = 100;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& sm) {
    sm.get(Coordinates{5, 7}).set_owner(2);
    sm.get(Coordinates{5, 7}).set_popn_exact(1000);
    sm.get(Coordinates{5, 7}).set_troops(500);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() += 1000;
    planet.troops() += 500;
    planet.info(player_t{2}).popn += 1000;
    planet.info(player_t{2}).troops += 500;
    planet.info(player_t{2}).numsectsowned += 1;
  });

  g.out.str("");
  // Send 1 civilian to attack 1000 civ + 500 troops with max tech
  ctx.assert_dispatch_success(g, {"move", "5,6", "k", "1"});
  test::expect_contains(g.out.str(), "killed your party to the last man");

  ctx.verify_universe_invariants();
}

void test_move_popn_command_matrix() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  TestCommandMatrix(ctx, "move")
      .with_valid_argv({"move", "5,5", "k", "10"})
      .with_invalid_argv({"move", "5,5"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .run_matrix(g);

  TestCommandMatrix(ctx, "deploy")
      .with_valid_argv({"deploy", "5,5", "k", "10"})
      .with_invalid_argv({"deploy", "5,5"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

void test_move_popn_military_assault() {
  TestContext ctx;
  setup_test_world(ctx);

  // Configure attacker (P1) with strong military stats
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 50.0;
    r.tech = 100.0;
    r.morale = 150;
    r.absorb = true;
    r.likes[SectorType::SEC_LAND] = 50;
  });

  // Configure defender (P2) with weak stats
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1.0;
    r.tech = 10.0;
    r.morale = 20;
    r.absorb = false;
    r.likes[SectorType::SEC_LAND] = 10;
  });

  // Setup sectors: (5,5) has 500 civs, 500 troops P1; (5,6) has 10 civs, 10
  // troops P2
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(500);
    smap.get(Coordinates{5, 5}).set_troops(500);

    smap.get(Coordinates{5, 6}).set_owner(2);
    smap.get(Coordinates{5, 6}).set_popn_exact(10);
    smap.get(Coordinates{5, 6}).set_troops(10);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() = 510;
    planet.troops() = 510;
    planet.info(player_t{1}).popn = 500;
    planet.info(player_t{1}).troops = 500;
    planet.info(player_t{1}).numsectsowned = 1;
    planet.info(player_t{2}).popn = 10;
    planet.info(player_t{2}).troops = 10;
    planet.info(player_t{2}).numsectsowned = 1;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Deploy assault victory
  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "200"});
  test::expect_contains(g.out.str(), "VICTORY");
  test::expect_contains(g.out.str(), "mil assault");

  ctx.em.clear_cache();
  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(smap != nullptr);
  test::expect_eq(smap->get(Coordinates{5, 6}).get_owner(), 1);
  test::expect_ge(smap->get(Coordinates{5, 6}).get_troops(), 1);
  ctx.verify_universe_invariants();

  // Test military assault repulse/defeat
  // Setup strong defender P2 at (5,7)
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 100.0;
    r.tech = 200.0;
    r.morale = 200;
    r.absorb = false;
    r.likes[SectorType::SEC_LAND] = 100;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& sm) {
    sm.get(Coordinates{5, 7}).set_owner(2);
    sm.get(Coordinates{5, 7}).set_popn_exact(1000);
    sm.get(Coordinates{5, 7}).set_troops(500);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() += 1000;
    planet.troops() += 500;
    planet.info(player_t{2}).popn += 1000;
    planet.info(player_t{2}).troops += 500;
    planet.info(player_t{2}).numsectsowned += 1;
  });

  g.out.str("");
  ctx.assert_dispatch_success(g, {"deploy", "5,6", "k", "10"});
  test::expect_contains(g.out.str(), "repulsed");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_move_popn_happy_paths();
  test_move_popn_insufficient_ap();
  test_move_popn_role_and_scope_rejections();
  test_move_popn_domain_errors();
  test_move_popn_assault_and_unowned();
  test_move_popn_negative_and_default_counts();
  test_move_popn_multistep_path();
  test_move_popn_enslaved_and_origin_validations();
  test_move_popn_assault_metamorph_and_wiped();
  test_move_popn_military_assault();
  test_move_popn_command_matrix();

  std::println(std::cout, "✓ move_popn_test passed!");
  return 0;
}
