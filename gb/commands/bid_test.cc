// SPDX-License-Identifier: Apache-2.0

/// \file bid_test.cc
/// \brief Unit tests for the commodities market bidding system.

import dallib;
import gblib;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Bidder";
  race1.Guest = false;
  race1.governor[0].active = true;
  race1.governor[0].money = 10000;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Seller";
  race2.Guest = false;
  race2.governor[0].active = true;
  race2.governor[0].money = 5000;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "MarketHub";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.AP[0] = 100;
  ss.governor[0] = 0;
  ss.pnames.emplace_back("MarketPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.dimensions = {10, 10};
  ps.info[player_t{1}].explored = true;
  ps.info[player_t{1}].numsectsowned = 5;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a space port for bidding
  Ship port{};
  port.number() = 1;
  port.owner() = 1;
  port.governor() = 0;
  port.alive() = true;
  port.active() = true;
  port.type() = ShipType::OTYPE_GOV;  // Has ABIL_PORT capability
  port.damage() = 0.0;
  port.whatorbits() = ScopeLevel::LEVEL_PLAN;
  port.storbits() = 0;
  port.pnumorbits() = 0;

  ShipRepository ships_repo(store);
  ships_repo.save(port);

  // Link ship to planet
  {
    auto planet_handle = ctx.em.get_planet(0, 0);
    auto& p = *planet_handle;
    p.ships() = 1;
  }

  // Create a commodity lot for sale using Repository
  CommodRepository commod_repo(store);
  {
    Commod commod{};
    commod.id = 1;
    commod.owner = 2;  // Player 2 is selling
    commod.governor = 0;
    commod.type = CommodType::RESOURCE;
    commod.amount = 100;
    commod.deliver = false;
    commod.bid = 500;  // Minimum bid
    commod.bidder = 0;
    commod.star_from = 0;
    commod.planet_from = 0;
    commod.star_to = 0;
    commod.planet_to = 0;
    commod_repo.save(commod);
  }

  // Create GameObj for player 1 (bidder)
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println(std::cout, "List all commodities");
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"bid"});
    test::expect_contains(g.out.str(), "Galactic Bloodshed Commodities Market");
    std::println(std::cout, "✓ Listing all commodities succeeded");
  }

  std::println(std::cout, "List commodities by type");
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"bid", "r"});
    test::expect_contains(g.out.str(), "Galactic Bloodshed Commodities Market");
    std::println(std::cout, "✓ Listing commodities by type succeeded");
  }

  std::println(std::cout, "Place initial bid on commodity");
  {
    const auto* c_before = ctx.em.peek_commod(1);
    std::println(std::cout, "  Before: bid={}, bidder={}", c_before->bid,
                 c_before->bidder);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"bid", "1", "1000"});
    std::println(std::cout, "  Output: {}", g.out.str());

    ctx.em.clear_cache();
    const auto* c_after = ctx.em.peek_commod(1);
    std::println(std::cout, "  After: bid={}, bidder={}", c_after->bid,
                 c_after->bidder);
    test::expect_eq(c_after->bid, 1000);
    test::expect_eq(c_after->bidder, 1);
    test::expect_eq(c_after->bidder_gov, 0);
    test::expect_eq(c_after->star_to, 0);
    test::expect_eq(c_after->planet_to, 0);
    std::println(std::cout, "✓ Initial bid placed successfully");
  }

  std::println(std::cout, "Raise existing bid");
  {
    const auto* c_before = ctx.em.peek_commod(1);
    int previous_bid = c_before->bid;

    // Need to bid at least (1 + UP_BID) times the current bid
    int new_bid = (int)((double)previous_bid * (1.0 + UP_BID)) + 10;

    g.out.str("");
    ctx.assert_dispatch_success(g, {"bid", "1", std::to_string(new_bid)});

    ctx.em.clear_cache();
    const auto* c_after = ctx.em.peek_commod(1);
    test::expect_eq(c_after->bid, new_bid);
    test::expect_eq(c_after->bidder, 1);
    std::println(std::cout, "✓ Bid raised successfully");
  }

  std::println(std::cout, "Cannot bid less than minimum");
  {
    const auto* c_before = ctx.em.peek_commod(1);
    int previous_bid = c_before->bid;

    // Try to bid less than required
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"bid", "1", "100"});

    // Bid should not change
    ctx.em.clear_cache();
    const auto* c_after = ctx.em.peek_commod(1);
    test::expect_eq(c_after->bid, previous_bid);
    std::println(std::cout, "✓ Low bid rejected");
  }

  std::println(std::cout, "Guest race cannot bid");
  {
    // Make player 1 a guest
    {
      auto race_handle = ctx.em.get_race(1);
      auto& r = *race_handle;
      r.Guest = true;
    }

    const auto* c_before = ctx.em.peek_commod(1);
    int previous_bid = c_before->bid;

    auto& registry = get_test_session_registry();
    GameObj g2(ctx.em, registry);
    ctx.setup_game_obj(g2);
    g2.set_level(ScopeLevel::LEVEL_PLAN);
    g2.set_snum(0);
    g2.set_pnum(0);

    g2.out.str("");
    ctx.assert_dispatch_rejected(g2, {"bid", "1", "5000"});
    test::expect_contains(g2.out.str(), "Guest races cannot bid.");

    // Bid should not change
    ctx.em.clear_cache();
    const auto* c_after = ctx.em.peek_commod(1);
    test::expect_eq(c_after->bid, previous_bid);
    std::println(std::cout, "✓ Guest race blocked from bidding");
  }

  std::println(std::cout, "All bid tests passed!");
  return 0;
}
