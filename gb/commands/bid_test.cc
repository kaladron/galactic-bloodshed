// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

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
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.info[0].explored = true;
  ps.info[0].numsectsowned = 5;
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
    auto planet_handle = em.get_planet(0, 0);
    auto& p = *planet_handle;
    p.ships() = 1;
  }

  // Create a commodity lot for sale
  Commod commod{};
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

  // Save commodity using legacy putcommod (commodities not yet in
  // EntityManager)
  putcommod(commod, 1);

  // Create GameObj for player 1 (bidder)
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 0;
  g.pnum = 0;

  std::println("Test 1: Place initial bid on commodity");
  {
    auto c_before = getcommod(1);
    std::println("  Before: bid={}, bidder={}", c_before.bid, c_before.bidder);

    command_t argv = {"bid", "1", "1000"};
    GB::commands::bid(argv, g);
    std::println("  Output: {}", g.out.str());

    auto c_after = getcommod(1);
    std::println("  After: bid={}, bidder={}", c_after.bid, c_after.bidder);
    assert(c_after.bid == 1000);
    assert(c_after.bidder == 1);
    assert(c_after.bidder_gov == 0);
    assert(c_after.star_to == 0);
    assert(c_after.planet_to == 0);
    std::println("✓ Initial bid placed successfully");
  }

  std::println("Test 2: Raise existing bid");
  {
    auto c_before = getcommod(1);
    int previous_bid = c_before.bid;

    // Need to bid at least (1 + UP_BID) times the current bid
    int new_bid = (int)((double)previous_bid * (1.0 + UP_BID)) + 10;

    command_t argv = {"bid", "1", std::to_string(new_bid)};
    GB::commands::bid(argv, g);

    auto c_after = getcommod(1);
    assert(c_after.bid == new_bid);
    assert(c_after.bidder == 1);
    std::println("✓ Bid raised successfully");
  }

  std::println("Test 3: Cannot bid less than minimum");
  {
    auto c_before = getcommod(1);
    int previous_bid = c_before.bid;

    // Try to bid less than required
    command_t argv = {"bid", "1", "100"};
    GB::commands::bid(argv, g);

    // Bid should not change
    auto c_after = getcommod(1);
    assert(c_after.bid == previous_bid);
    std::println("✓ Low bid rejected");
  }

  std::println("Test 4: Guest race cannot bid");
  {
    // Make player 1 a guest
    {
      auto race_handle = em.get_race(1);
      auto& r = *race_handle;
      r.Guest = true;
    }

    auto c_before = getcommod(1);
    int previous_bid = c_before.bid;

    GameObj g2(em);
    g2.player = 1;
    g2.governor = 0;
    g2.race = em.peek_race(1);
    g2.level = ScopeLevel::LEVEL_PLAN;
    g2.snum = 0;
    g2.pnum = 0;

    command_t argv = {"bid", "1", "5000"};
    GB::commands::bid(argv, g2);

    // Bid should not change
    auto c_after = getcommod(1);
    assert(c_after.bid == previous_bid);
    std::println("✓ Guest race blocked from bidding");
  }

  std::println("All bid tests passed!");
  return 0;
}
