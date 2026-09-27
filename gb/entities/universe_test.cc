// SPDX-License-Identifier: Apache-2.0

/// \file universe_test.cc
/// \brief Unit tests for VnTargetRecord, universe_struct Action Points (AP),
/// Von Neumann (VN) tracking, and EntityManager persistence.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import test;
import std;

void test_vn_target_record() {
  std::println(std::cout, "Test: VnTargetRecord destruction and retaliation");

  VnTargetRecord record{};
  test::expect_eq(record.hits, 0U);
  test::expect_eq(record.primary_star, std::nullopt);
  test::expect_eq(record.secondary_star, std::nullopt);
  test::expect_eq(record.select_retaliation_star(true), std::nullopt);
  test::expect_eq(record.select_retaliation_star(false), std::nullopt);

  // Invalid star ID 0 throws std::out_of_range
  test::expect_throws<std::out_of_range>(
      [&]() { record.record_destruction_star(0, false); });

  // First star populates primary_star regardless of replace_primary
  record.record_destruction_star(10, false);
  test::expect_eq(record.primary_star, starnum_t{10});
  test::expect_eq(record.secondary_star, std::nullopt);
  test::expect_eq(record.select_retaliation_star(true), starnum_t{10});
  test::expect_eq(record.select_retaliation_star(false), starnum_t{10});

  // Second star populates secondary_star regardless of replace_primary
  record.record_destruction_star(20, true);
  test::expect_eq(record.primary_star, starnum_t{10});
  test::expect_eq(record.secondary_star, starnum_t{20});
  test::expect_eq(record.select_retaliation_star(true), starnum_t{10});
  test::expect_eq(record.select_retaliation_star(false), starnum_t{20});

  // When both are occupied, replace_primary=true overwrites primary_star
  record.record_destruction_star(30, true);
  test::expect_eq(record.primary_star, starnum_t{30});
  test::expect_eq(record.secondary_star, starnum_t{20});

  // When both are occupied, replace_primary=false overwrites secondary_star
  record.record_destruction_star(40, false);
  test::expect_eq(record.primary_star, starnum_t{30});
  test::expect_eq(record.secondary_star, starnum_t{40});

  // Fallback when only secondary_star is set
  VnTargetRecord secondary_only{
      .hits = 1, .primary_star = std::nullopt, .secondary_star = starnum_t{55}};
  test::expect_eq(secondary_only.select_retaliation_star(true), starnum_t{55});
  test::expect_eq(secondary_only.select_retaliation_star(false), starnum_t{55});

  std::println(std::cout, "  ✓ VnTargetRecord methods work correctly");
}

void test_universe_AP_methods() {
  std::println(std::cout, "Test: universe_struct AP (Action Points) methods");

  universe_struct universe{};
  const auto& const_universe = universe;

  // Non-inserting const lookup returns 0
  test::expect_eq(const_universe.get_AP(1), 0);
  test::expect_true(const_universe.AP.empty());

  // Set AP for player 1
  universe.set_AP(1, 1000);
  test::expect_eq(const_universe.get_AP(1), 1000);

  // Deduct AP
  universe.deduct_AP(1, 300);
  test::expect_eq(const_universe.get_AP(1), 700);

  // Deduct more than available (should clamp to 0 and erase sparse entry)
  universe.deduct_AP(1, 1000);
  test::expect_eq(const_universe.get_AP(1), 0);
  test::expect_false(const_universe.AP.contains(1));

  // Add AP
  universe.add_AP(1, 500);
  test::expect_eq(const_universe.get_AP(1), 500);

  // Test multiple players
  universe.set_AP(2, 2000);
  universe.set_AP(3, 3000);
  test::expect_eq(const_universe.get_AP(2), 2000);
  test::expect_eq(const_universe.get_AP(3), 3000);
  test::expect_eq(const_universe.get_AP(1), 500);

  // Invalid player ID 0 throws std::out_of_range
  test::expect_throws<std::out_of_range>(
      [&]() { (void)const_universe.get_AP(0); });
  test::expect_throws<std::out_of_range>([&]() { universe.set_AP(0, 999); });

  std::println(std::cout, "  ✓ AP methods work correctly");
}

void test_universe_VN_methods() {
  std::println(std::cout,
               "Test: universe_struct VN (Von Neumann) tracking methods");

  universe_struct universe{};
  const auto& const_universe = universe;

  // Non-inserting const lookup returns default record
  test::expect_eq(const_universe.vn_hits(1), 0U);
  test::expect_eq(const_universe.vn_target(1), VnTargetRecord{});
  test::expect_true(const_universe.vn_targets.empty());

  // Record VN kill in deep space (no star)
  universe.record_vn_kill(1, std::nullopt, false);
  test::expect_eq(const_universe.vn_hits(1), 1U);
  test::expect_eq(const_universe.vn_target(1).primary_star, std::nullopt);
  test::expect_eq(const_universe.vn_target(1).secondary_star, std::nullopt);

  // Record VN kills in star systems
  universe.record_vn_kill(1, starnum_t{42}, false);
  test::expect_eq(const_universe.vn_hits(1), 2U);
  test::expect_eq(const_universe.vn_target(1).primary_star, starnum_t{42});
  test::expect_eq(const_universe.vn_target(1).secondary_star, std::nullopt);

  universe.record_vn_kill(1, starnum_t{100}, false);
  test::expect_eq(const_universe.vn_hits(1), 3U);
  test::expect_eq(const_universe.vn_target(1).primary_star, starnum_t{42});
  test::expect_eq(const_universe.vn_target(1).secondary_star, starnum_t{100});

  // Decrement VN hits
  universe.decrement_vn_hits(1);
  test::expect_eq(const_universe.vn_hits(1), 2U);

  // Decrement at 0 or missing player should not underflow
  universe.decrement_vn_hits(2);
  test::expect_eq(const_universe.vn_hits(2), 0U);

  universe.vn_target(2).hits = 1;
  universe.decrement_vn_hits(2);
  test::expect_eq(const_universe.vn_hits(2), 0U);
  universe.decrement_vn_hits(2);
  test::expect_eq(const_universe.vn_hits(2), 0U);

  // Invalid player ID 0 or star ID 0 throws std::out_of_range
  test::expect_throws<std::out_of_range>(
      [&]() { (void)const_universe.vn_hits(0); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)universe.vn_target(0); });
  test::expect_throws<std::out_of_range>(
      [&]() { universe.record_vn_kill(0, starnum_t{1}, false); });
  test::expect_throws<std::out_of_range>(
      [&]() { universe.record_vn_kill(1, starnum_t{0}, false); });
  test::expect_throws<std::out_of_range>(
      [&]() { universe.decrement_vn_hits(0); });

  std::println(std::cout, "  ✓ VN tracking methods work correctly");
}

void test_universe_persistence() {
  std::println(std::cout, "Test: Universe persistence with EntityManager");

  Database db(":memory:");
  initialize_schema(db);

  // Create initial universe data in database (singleton with id=1)
  {
    JsonStore store(db);
    UniverseRepository repo(store);

    universe_struct u{};
    u.set_AP(1, 1000);
    u.set_AP(2, 2000);
    u.vn_target(1) = VnTargetRecord{.hits = 5,
                                    .primary_star = starnum_t{3},
                                    .secondary_star = starnum_t{7}};

    repo.save(u);
  }

  // Now use EntityManager to retrieve and verify
  EntityManager em(db);
  const auto* universe = em.peek_universe();
  test::expect_ne(universe, nullptr);
  test::expect_eq(universe->get_AP(1), 1000);
  test::expect_eq(universe->get_AP(2), 2000);
  test::expect_eq(universe->vn_hits(1), 5U);
  test::expect_eq(universe->vn_target(1).primary_star, starnum_t{3});
  test::expect_eq(universe->vn_target(1).secondary_star, starnum_t{7});

  // Modify via EntityManager
  em.mutate_universe([](universe_struct& universe_mut) {
    universe_mut.set_AP(1, 1500);
    universe_mut.record_vn_kill(1, starnum_t{9}, true);
  });

  // Clear cache to force reload from DB
  em.clear_cache();

  // Retrieve and verify modification
  const auto* universe2 = em.peek_universe();
  test::expect_ne(universe2, nullptr);
  test::expect_eq(universe2->get_AP(1), 1500);
  test::expect_eq(universe2->get_AP(2), 2000);
  test::expect_eq(universe2->vn_hits(1), 6U);
  test::expect_eq(universe2->vn_target(1).primary_star, starnum_t{9});
  test::expect_eq(universe2->vn_target(1).secondary_star, starnum_t{7});

  std::println(std::cout, "  ✓ Persistence with EntityManager works correctly");
}

int main() {
  test_vn_target_record();
  test_universe_AP_methods();
  test_universe_VN_methods();
  test_universe_persistence();

  std::println(std::cout, "\n✅ All Universe tests passed!");
  return 0;
}
