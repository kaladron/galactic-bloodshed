// SPDX-License-Identifier: Apache-2.0

/// \file dissolve_test.cc
/// \brief Unit tests for dissolve command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

constexpr player_t p1 = 2;
constexpr player_t p2 = 3;
constexpr player_t guest_player = 4;

std::pair<shipnum_t, shipnum_t> setup_test_world(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, p1);
  world.add_race("Klingons", 100.0, false, p2);
  ctx.with_populated_planet(1, 1, p1, 1000, Coordinates{0, 0})
      .with_populated_planet(2, 1, p2, 1000, Coordinates{0, 0});

  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(p1);
    s.mark_inhabited_by(p1);
    s.AP(p1) = 50;
  });
  ctx.em.mutate_star(2, [](Star& s) {
    s.mark_explored_by(p2);
    s.mark_inhabited_by(p2);
    s.AP(p2) = 50;
  });

  ctx.em.mutate_race(p1, [](Race& race) {
    race.password = "testpass";
    race.leader().password = "govpass";
    race.appoint_governor(2, {.password = "subpass"});
    race.dissolved = false;
  });

  ctx.em.mutate_race(p2, [](Race& race2) {
    race2.password = "otherpass";
    race2.leader().password = "othergov";
  });

  const auto s1 = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                      .owned_by(p1, 1)
                      .in_planet_orbit(1, 1)
                      .build();
  const auto s2 = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                      .owned_by(p2, 1)
                      .in_planet_orbit(1, 1)
                      .build();

  // Load race into EntityManager cache to ensure getracenum can find it
  const auto* loaded_race = ctx.em.peek_race(p1);
  test::expect_ne(loaded_race, nullptr);
  test::expect_eq(loaded_race->password, "testpass");
  test::expect_eq(loaded_race->leader().password, "govpass");

  // Initialize Earth sector (0, 0) with nonzero efficiency and mobilization
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    auto& s = smap.get({0, 0});
    s.set_efficiency_bounded(85);
    s.set_mobilization(50);
  });

  return {s1, s2};
}

void test_dissolve_happy_path() {
  TestContext ctx;
  const auto [ship1, ship2] = setup_test_world(ctx);

  // Seed foreign slaved_to, market lots/bids, coalition blocks, and Power stats
  ctx.em.mutate_planet(2, 1, [](Planet& p) { p.slaved_to() = p1; });
  int own_lot_id = 0;
  int bid_lot_id = 0;
  {
    auto own_lot = ctx.em.create_commod({.owner = p1,
                                         .governor = 1,
                                         .amount = 10,
                                         .star_from = 1,
                                         .planet_from = 1});
    own_lot_id = own_lot->id();
    own_lot.save();
    auto bid_lot = ctx.em.create_commod({.owner = p2,
                                         .governor = 1,
                                         .amount = 15,
                                         .star_from = 2,
                                         .planet_from = 1});
    bid_lot->place_bid(p1, 1, 200, 1, 1);
    bid_lot_id = bid_lot->id();
    bid_lot.save();
  }
  ctx.em.mutate_block(p1, [](Block& b) {
    b.add_member(p2);
    b.VPs() = 50;
  });
  ctx.em.mutate_block(p2, [](Block& b) { b.add_member(p1); });
  ctx.em.mutate_power(p1, [](Power& p) {
    p = Power{{.id = p1, .popn = 1000, .ships_owned = 1}};
  });

  // Create GameObj for command execution with co-governor online
  RecordingSessionRegistry registry;
  registry.sessions = {
      SessionInfo{.player = p1, .governor = 1, .connected = true},
      SessionInfo{.player = p1, .governor = 2, .connected = true},
      SessionInfo{.player = p2, .governor = 1, .connected = true},
  };
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_univ();

  std::println(std::cout, "Dissolve race with correct passwords and waste");
  {
    ctx.assert_dispatch_success(g,
                                {"dissolve", "testpass", "govpass", "waste"});
    test::expect_contains(
        g.out.str(), std::format("Ship #{}, self-destruct enabled", ship1));
    test::expect_true(g.disconnect_requested());
    test::expect_false(registry.is_connected(p1, 2));
    test::expect_true(registry.is_connected(p2, 1));
    test::expect_true(
        registry.has_received(p1, "Your empire has been dissolved."));

    // Clear cache to force reload from database
    ctx.em.clear_cache();

    // Verify race was dissolved
    const auto* saved_race = ctx.em.peek_race(p1);
    test::expect_ne(saved_race, nullptr);
    test::expect_true(saved_race->dissolved);

    // Verify foreign enslavement, market lots/bids, Block, and Power scrubbed
    test::expect_eq(ctx.em.peek_planet(2, 1)->slaved_to(), std::nullopt);
    test::expect_throws<EntityNotFoundError>(
        [&]() { ctx.em.peek_commod(own_lot_id); });
    test::expect_false(ctx.em.peek_commod(bid_lot_id)->has_bid());
    test::expect_eq(ctx.em.peek_block(p1)->members(), 0U);
    test::expect_true(ctx.em.peek_block(p1)->invited().empty());
    test::expect_false(ctx.em.peek_block(p2)->is_invited(p1));
    test::expect_false(ctx.em.peek_block(p2)->is_pledged(p1));
    test::expect_eq(ctx.em.peek_power(p1)->popn(), 0);

    // Verify ship was destroyed (hard-deleted) while p2's ship remains alive
    test::expect_throws<EntityNotFoundError>(
        [&]() { ctx.em.peek_ship(ship1); });
    const auto* saved_ship2 = ctx.em.peek_ship(ship2);
    test::expect_ne(saved_ship2, nullptr);
    test::expect_true(saved_ship2->alive());

    // Verify Earth sector (0,0) was cleared and wasted, planet demographics
    // synced, and star inhabitation cleared for p1
    const auto* smap = ctx.em.peek_sectormap(1, 1);
    test::expect_eq(smap->get({0, 0}).get_owner(), 0);
    test::expect_eq(smap->get({0, 0}).get_popn(), 0);
    test::expect_eq(smap->get({0, 0}).get_troops(), 0);
    test::expect_eq(smap->get({0, 0}).get_eff(), 0);
    test::expect_eq(smap->get({0, 0}).get_mobilization(), 0);
    test::expect_eq(smap->get({0, 0}).get_condition(), SectorType::SEC_WASTED);

    const auto* pl = ctx.em.peek_planet(1, 1);
    test::expect_eq(pl->popn(), 0);
    test::expect_eq(pl->troops(), 0);
    test::expect_eq(pl->info(p1).numsectsowned, 0);

    const auto* star0 = ctx.em.peek_star(1);
    test::expect_false(star0->is_inhabited_by(p1));

    // Verify Vega Prime (p2 colony) remains intact
    const auto* vega_pl = ctx.em.peek_planet(2, 1);
    test::expect_eq(vega_pl->popn(), 1000);
    test::expect_eq(vega_pl->info(p2).numsectsowned, 1);

    ctx.verify_universe_invariants();
  }
}

void test_dissolve_role_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  Race guest_race{};
  guest_race.Playernum = guest_player;
  guest_race.name = "GuestRace";
  guest_race.password = "guestpass";
  guest_race.Guest = true;
  guest_race.leader().password = "guestgov";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, guest_player, 1);
  g.set_scope_univ();
  ctx.assert_dispatch_rejected(g, {"dissolve", "guestpass", "guestgov"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Leader-only rejection (Governor 2 via dispatcher)
  g.out.str("");
  ctx.setup_game_obj(g, p1, 2);
  g.set_scope_univ();
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass", "subpass"});
  test::expect_contains(g.out.str(), "leader (Governor 1)");

  // 3. Direct handler governor != 1 leader notification check
  g.out.str("");
  test::expect_false(
      GB::commands::dissolve({"dissolve", "testpass", "subpass"}, g));
  test::expect_contains(g.out.str(), "The leader has been notified");
}

void test_dissolve_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_univ();

  // 0. Feature switch disabled (dissolve_enabled = false)
  test::expect_false(GB::commands::dissolve({"dissolve", "testpass", "govpass"},
                                            g, /*dissolve_enabled=*/false));
  test::expect_contains(g.out.str(), "Dissolve has been disabled.");
  test::expect_false(ctx.em.peek_race(p1)->dissolved);
  g.out.str("");

  // 1. Min args (< 3 args via dispatcher and direct handler)
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass"});
  test::expect_contains(
      g.out.str(),
      "Syntax: dissolve <race password> <leader password> [waste]");
  g.out.str("");
  test::expect_false(GB::commands::dissolve({"dissolve", "testpass"}, g));
  test::expect_contains(g.out.str(),
                        "Self-Destruct sequence requires passwords.");

  // 2. Password mismatch (non-existent credentials)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "wrongpass", "wronggov"});
  test::expect_contains(g.out.str(), "Password mismatch");

  // 3. Cross-player password rejection (p1 supplying p2's valid leader
  // credentials must be rejected!)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "otherpass", "othergov"});
  test::expect_contains(g.out.str(), "Password mismatch");
  test::expect_false(ctx.em.peek_race(p1)->dissolved);

  // 4. Subordinate governor password rejection (p1 supplying their own
  // Governor 2 password instead of Governor 1 leader password)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass", "subpass"});
  test::expect_contains(g.out.str(), "Password mismatch");
  test::expect_false(ctx.em.peek_race(p1)->dissolved);
}

void test_dissolve_without_waste_and_json() {
  // 1. 3-arg dissolve without waste (argv.size() == 3)
  {
    TestContext ctx;
    setup_test_world(ctx);

    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, p1, 1);
    g.set_scope_univ();

    const auto orig_cond =
        ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_condition();
    ctx.assert_dispatch_success(g, {"dissolve", "testpass", "govpass"});
    ctx.em.clear_cache();
    test::expect_true(ctx.em.peek_race(p1)->dissolved);
    test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_condition(),
                    orig_cond);
    test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_eff(), 0);
    test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_mobilization(),
                    0);
    test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_owner(), 0);
    test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_popn(), 0);
    test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({0, 0}).get_troops(), 0);
    ctx.verify_universe_invariants();
  }

  // 2. 4-arg dissolve with non-'w' option in UiMode::JSON
  {
    TestContext ctx;
    setup_test_world(ctx);

    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, p1, 1);
    g.set_scope_univ();
    g.set_ui_mode(UiMode::JSON);

    ctx.assert_dispatch_success(g, {"dissolve", "testpass", "govpass", "keep"});
    test::expect_contains(g.out.str(), R"("type":"message")");
    test::expect_contains(g.out.str(), "Ship #1, self-destruct enabled");
    test::expect_contains(g.out.str(), R"("type":"race")");
    test::expect_contains(g.out.str(), R"("dissolved":true)");
    ctx.verify_universe_invariants();
  }
}

}  // namespace

int main() {
  test_dissolve_happy_path();
  test_dissolve_role_rejections();
  test_dissolve_domain_errors();
  test_dissolve_without_waste_and_json();

  std::println(std::cout, "\n✅ All dissolve tests passed!");
  return 0;
}
