// SPDX-License-Identifier: Apache-2.0

/// \file revoke_test.cc
/// \brief Unit tests for revoke command.

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_revoke_validation_and_transfers() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.leader().password = "leadpass";
  race.appoint_governor(2, {.password = "secret123", .money = 450});
  race.appoint_governor(3, {.password = "secret789", .money = 100});
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  ctx.assert_dispatch_rejected(g, {"revoke", "2"});
  test::expect_contains(g.out.str(),
                        "Syntax: revoke <gov> <password> [<target>]");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "abc", "secret123"});
  test::expect_contains(g.out.str(), "No such governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "0", "secret123"});
  test::expect_contains(g.out.str(), "No such governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "1", "leadpass"});
  test::expect_contains(g.out.str(), "You can't revoke your leadership!");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "secret123", "abc"});
  test::expect_contains(g.out.str(), "You can't give stuff to that governor!");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "secret123", "0"});
  test::expect_contains(g.out.str(), "You can't give stuff to that governor!");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "9", "secret123"});
  test::expect_contains(g.out.str(), "That governor is not active");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "wrongpass"});
  test::expect_contains(g.out.str(), "Incorrect password");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "secret123", "9"});
  test::expect_contains(g.out.str(), "Bad target governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "secret123", "2"});
  test::expect_contains(g.out.str(), "Bad target governor");

  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  ctx.assert_dispatch_rejected(g, {"revoke", "3", "secret789"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command");

  ctx.setup_game_obj(g, 1, 1);
  star_struct s{};
  s.star_id = 1;
  s.name = "Sol";
  s.pnames.push_back("Earth");
  s.governor[1] = 2;
  Planet p(1, 1, PlanetType::EARTH);
  {
    JsonStore store(ctx.db);
    StarRepository stars(store);
    stars.save(Star(s));
    PlanetRepository planets(store);
    planets.save(p);
  }
  {
    auto ship1 = ctx.em.create_ship(ShipType::OTYPE_FACTORY, 1);
    ship1->alive() = 1;
    ship1.save();
    auto ship2 = ctx.em.create_ship(ShipType::OTYPE_FACTORY, 1);
    ship2->governor() = 2;
    ship2->alive() = 1;
    ship2.save();
  }
  {
    auto barrier = ctx.em.create_deletion_barrier();
    ctx.em.delete_ship(1);
  }
  {
    auto commod =
        ctx.em.create_commod(commod_struct{.owner = 1,
                                           .governor = 2,
                                           .type = CommodType::RESOURCE,
                                           .amount = 20,
                                           .star_from = 1,
                                           .planet_from = 1});
    commod.save();
  }

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"revoke", "2", "secret123", "3"});
  test::expect_false(ctx.em.peek_race(1)->has_governor(2));
  test::expect_eq(ctx.em.peek_race(1)->governor(3).money, money_t{550});
  test::expect_eq(ctx.em.peek_star(1)->governor(1), governor_t{3});
  test::expect_eq(ctx.em.peek_ship(2)->governor(), governor_t{3});
  test::expect_eq(ctx.em.peek_commod(1)->governor(), governor_t{3});
  test::expect_contains(g.out.str(), "\"type\":\"revoke\"");
  test::expect_contains(g.out.str(), "\"revoked_governor\":2");
  test::expect_contains(g.out.str(), "\"target_governor\":3");
  test::expect_contains(g.out.str(), "\"money_transferred\":450");

  g.set_ui_mode(UiMode::ASCII);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"revoke", "3", "secret789"});
  test::expect_contains(g.out.str(), "Done.");
  test::expect_false(ctx.em.peek_race(1)->has_governor(3));
  test::expect_eq(ctx.em.peek_race(1)->leader().money, money_t{550});

  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "secret123"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command");
}

}  // namespace

int main() {
  test_revoke_validation_and_transfers();

  std::println(std::cout, "✓ revoke_test passed!");
  return 0;
}
