// SPDX-License-Identifier: Apache-2.0

/// \file commod_json_test.cc
/// \brief Unit tests for Commod entity SQLite JSON serialization and
/// EntityManager integration.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  TestContext ctx;
  ctx.with_races();

  // Create JsonStore and Repository for initial save
  JsonStore store(ctx.db);
  PlanetRepository(store).save(Planet{1, 2});
  PlanetRepository(store).save(Planet{2, 3});
  CommodRepository commod_repo(store);

  Commod test_commod{{
      .id = 42,
      .owner = 1,
      .governor = 2,
      .type = CommodType::RESOURCE,
      .amount = 500,
      .deliver = true,
      .bid = 100,
      .bidder = 3,
      .bidder_gov = 4,
      .star_from = 1,
      .planet_from = 2,
      .star_to = 2,
      .planet_to = 3,
  }};

  int commodnum = 42;

  // Test Repository::save - stores in SQLite as JSON
  commod_repo.save(test_commod);

  // Test EntityManager::peek_commod - reads from SQLite
  const auto* retrieved_commod = ctx.em.peek_commod(commodnum);
  test::expect_ne(retrieved_commod, nullptr);

  // Verify key fields
  test::expect_eq(retrieved_commod->owner(), test_commod.owner());
  test::expect_eq(retrieved_commod->governor(), test_commod.governor());
  test::expect_eq(retrieved_commod->type(), test_commod.type());
  test::expect_eq(retrieved_commod->amount(), test_commod.amount());
  test::expect_eq(retrieved_commod->deliver(), test_commod.deliver());
  test::expect_eq(retrieved_commod->bid(), test_commod.bid());
  test::expect_eq(retrieved_commod->bidder(), test_commod.bidder());
  test::expect_eq(retrieved_commod->bidder_gov(), test_commod.bidder_gov());
  test::expect_eq(retrieved_commod->star_from(), test_commod.star_from());
  test::expect_eq(retrieved_commod->planet_from(), test_commod.planet_from());
  test::expect_eq(retrieved_commod->star_to(), test_commod.star_to());
  test::expect_eq(retrieved_commod->planet_to(), test_commod.planet_to());

  // Verify min_next_bid() normal scaling, non-positive guard, and INT64_MAX
  // saturation
  test::expect_eq(retrieved_commod->min_next_bid(),
                  static_cast<money_t>(100.0 * (1.0 + UP_BID)));
  Commod zero_bid_commod{{.id = 43, .amount = 10, .bid = 0}};
  test::expect_eq(zero_bid_commod.min_next_bid(), 0);
  Commod max_bid_commod{
      {.id = 44, .amount = 10, .bid = std::numeric_limits<money_t>::max()}};
  test::expect_eq(max_bid_commod.min_next_bid(),
                  std::numeric_limits<money_t>::max());

  // Database connection will be cleaned up automatically by Sql destructor

  std::println(std::cout, "Commod SQLite JSON storage test passed!");
  return 0;
}