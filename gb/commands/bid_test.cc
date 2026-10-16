// SPDX-License-Identifier: Apache-2.0

/// \file bid_test.cc
/// \brief Unit tests for the commodities market bidding system.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.create_planet(1, PlanetType::MARS, Coordinates{10, 10}, 2)
      .named("Mars")
      .build();

  ctx.em.mutate_race(1, [](Race& r) { r.leader().money = 10000; });
  ctx.em.mutate_race(2, [](Race& r) { r.leader().money = 5000; });

  // Create a space port for bidding on Sol / Earth (Star 1, Planet 1)
  const shipnum_t port_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                  .owned_by(1, 1)
                                  .in_planet_orbit(1, 1)
                                  .build();

  (void)ctx.em.create_commod({
      .owner = 2,
      .governor = 1,
      .type = CommodType::RESOURCE,
      .amount = 100,
      .deliver = false,
      .bid = 500,
      .star_from = 1,
      .planet_from = 1,
  });
  (void)ctx.em.create_commod({
      .owner = 1,
      .governor = 1,
      .type = CommodType::DESTRUCT,
      .amount = 50,
      .deliver = true,
      .bid = 0,
      .star_from = 1,
      .planet_from = 1,
  });
  (void)ctx.em.create_commod({
      .owner = 2,
      .governor = 1,
      .type = CommodType::FUEL,
      .amount = 200,
      .deliver = false,
      .bid = 100,
      .bidder = player_t{2},
      .bidder_gov = 1,
      .star_from = 1,
      .planet_from = 1,
      .star_to = starnum_t{1},
      .planet_to = planetnum_t{1},
  });
  (void)ctx.em.create_commod({
      .owner = 2,
      .governor = 1,
      .type = CommodType::CRYSTAL,
      .amount = 10,
      .deliver = false,
      .bid = 100,
      .star_from = 1,
      .planet_from = 1,
  });
  // Create and delete lot 5 to test missing lot lookup within 1..max_commod_id
  (void)ctx.em.create_commod({
      .owner = 2,
      .governor = 1,
      .type = CommodType::RESOURCE,
      .amount = 25,
      .bid = 10,
      .star_from = 1,
      .planet_from = 1,
  });
  {
    auto barrier = ctx.em.create_deletion_barrier();
    ctx.em.delete_commod(5);
  }
  (void)ctx.em.create_commod({
      .owner = 2,
      .governor = 1,
      .type = CommodType::RESOURCE,
      .amount = 25,
      .bid = 10,
      .star_from = 1,
      .planet_from = 1,
  });

  // Create GameObj for player 1 (bidder)
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  std::println(std::cout,
               "List all commodities at UNIV and STAR scopes, and by type");
  {
    ctx.assert_dispatch_success(g, {"bid"});
    test::expect_contains(g.out.str(), "Galactic Bloodshed Commodities Market");
    test::expect_contains(g.out.str(), "*");

    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);
    ctx.assert_dispatch_success(g, {"bid"});
    test::expect_contains(g.out.str(), "Galactic Bloodshed Commodities Market");

    ctx.assert_dispatch_rejected(g, {"bid", "1", "1000"});
    test::expect_contains(g.out.str(),
                          "You have to be in a planet scope to buy.");

    for (const char* t : {"r", "d", "f", "x"}) {
      ctx.assert_dispatch_success(g, {"bid", t});
      test::expect_contains(g.out.str(),
                            "Galactic Bloodshed Commodities Market");
    }
    ctx.assert_dispatch_rejected(g, {"bid", "z"});
    test::expect_contains(g.out.str(), "No such type of commodity.");
    ctx.assert_dispatch_rejected(g, {"bid", ""});
    test::expect_contains(g.out.str(), "No such type of commodity.");
  }

  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  std::println(std::cout, "Place initial bid on commodity and list in JSON");
  {
    ctx.assert_dispatch_success(g, {"bid", "1", "1000"});
    test::expect_contains(g.out.str(), "Bid accepted.");

    ctx.em.clear_cache();
    const auto* c_after = ctx.em.peek_commod(1);
    test::expect_eq(c_after->bid(), 1000);
    test::expect_eq(c_after->bidder(), player_t{1});
    test::expect_eq(c_after->bidder_gov(), 1);
    test::expect_eq(c_after->star_to(), starnum_t{1});
    test::expect_eq(c_after->planet_to(), planetnum_t{1});

    // Listing at PLAN scope now includes player 1's destination (Sol/Earth ->
    // "Sol /Eart")
    ctx.assert_dispatch_success(g, {"bid"});
    test::expect_contains(g.out.str(), "Sol /Eart");

    g.set_ui_mode(UiMode::JSON);
    ctx.assert_dispatch_success(g, {"bid", "r"});
    test::expect_contains(g.out.str(), "\"type\":\"commodity_market\"");
    g.set_ui_mode(UiMode::ASCII);
  }

  std::println(std::cout, "Raise existing bid in JSON mode");
  {
    const auto* c_before = ctx.em.peek_commod(1);
    int previous_bid = c_before->bid();
    int new_bid = static_cast<int>(previous_bid * (1.0 + UP_BID)) + 10;

    g.set_ui_mode(UiMode::JSON);
    ctx.assert_dispatch_success(g, {"bid", "1", std::to_string(new_bid)});
    test::expect_contains(g.out.str(), "\"type\":\"bid\"");
    g.set_ui_mode(UiMode::ASCII);

    ctx.em.clear_cache();
    const auto* c_after = ctx.em.peek_commod(1);
    test::expect_eq(c_after->bid(), new_bid);
    const auto outbid_teles = ctx.em.get_telegrams(1, 1);
    test::expect_false(outbid_teles.empty());
    test::expect_contains(outbid_teles.back().message,
                          "The bid on lot #1 (100 resources) has been upped");
  }

  std::println(std::cout, "Validation and error branches");
  {
    // Invalid lot / bid parsing and bounds
    ctx.assert_dispatch_rejected(g, {"bid", "abc", "1000"});
    test::expect_contains(g.out.str(), "Illegal lot number.");
    ctx.assert_dispatch_rejected(g, {"bid", "1", "abc"});
    test::expect_contains(g.out.str(), "Illegal lot number.");
    ctx.assert_dispatch_rejected(g, {"bid", "0", "1000"});
    test::expect_contains(g.out.str(), "Illegal lot number.");
    ctx.assert_dispatch_rejected(g, {"bid", "999", "1000"});
    test::expect_contains(g.out.str(), "Illegal lot number.");

    // Non-existent lot (id 5 within 1..max_commod_id)
    ctx.assert_dispatch_rejected(g, {"bid", "5", "1000"});
    test::expect_contains(g.out.str(), "No such lot for sale.");

    // Zero bid on a 0-bid lot and low bid on an existing bid
    ctx.assert_dispatch_rejected(g, {"bid", "2", "0"});
    test::expect_contains(g.out.str(), "You have to bid more than 0.");
    ctx.assert_dispatch_rejected(g, {"bid", "1", "100"});
    test::expect_contains(g.out.str(), "You have to bid more than");

    // Seller setting minimum price from same vs different planet/star
    ctx.assert_dispatch_success(g, {"bid", "2", "200"});
    ctx.em.mutate_commod(2, [](Commod& c) { c.set_origin(1, 2); });
    ctx.assert_dispatch_rejected(g, {"bid", "2", "300"});
    test::expect_contains(
        g.out.str(),
        "You can only set a minimum price for your lot from the location");
    ctx.em.mutate_commod(2, [](Commod& c) { c.set_origin(2, 1); });
    ctx.assert_dispatch_rejected(g, {"bid", "2", "300"});
    test::expect_contains(
        g.out.str(),
        "You can only set a minimum price for your lot from the location");

    // Insufficient funds (buying on credit)
    ctx.assert_dispatch_rejected(g, {"bid", "4", "999999"});
    test::expect_contains(g.out.str(), "Sorry, no buying on credit allowed.");

    // Damaged starport / no undamaged starport
    ctx.em.mutate_ship(port_ship, [](Ship& s) { s.admin_override_damage(50); });
    ctx.assert_dispatch_rejected(g, {"bid", "4", "500"});
    test::expect_contains(
        g.out.str(),
        "You don't have an undamaged space port or government center here.");
    ctx.em.mutate_ship(port_ship, [](Ship& s) { s.admin_override_damage(0); });

    // Enslaved planet
    ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = player_t{2}; });
    ctx.assert_dispatch_rejected(g, {"bid", "4", "500"});
    test::expect_contains(g.out.str(), "This planet is enslaved to player 2.");
    ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = std::nullopt; });

    // Unauthorized governor in star system
    g.set_governor(2);
    ctx.assert_dispatch_rejected(g, {"bid", "4", "500"});
    test::expect_contains(g.out.str(),
                          "You are not authorized in this system.");
    g.set_governor(1);

    // Guest race cannot bid
    ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
    GameObj g2(ctx.em, registry);
    ctx.setup_game_obj(g2);
    g2.set_level(ScopeLevel::LEVEL_PLAN);
    g2.set_snum(1);
    g2.set_pnum(1);
    ctx.assert_dispatch_rejected(g2, {"bid", "4", "500"});
    test::expect_contains(g2.out.str(), "Guest races cannot bid.");
  }

  std::println(std::cout, "All bid tests passed!");
  return 0;
}
