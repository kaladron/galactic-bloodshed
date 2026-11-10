// SPDX-License-Identifier: Apache-2.0

/// \file entity_manager_dock_test.cc
/// \brief Unit tests for EntityManager docking, unmooring, and 1-level carrier
/// hierarchy invariant enforcement.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import test;
import std;

namespace {

void expect_near(double actual, double expected, double eps = 1e-5) {
  test::expect_true(std::abs(actual - expected) <= eps,
                    std::format("Expected {} to be near {}, difference is {}",
                                actual, expected, std::abs(actual - expected)));
}

void test_dock_carrier_happy_path() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& em = ctx.em;

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_hanger(0)
                              .with_max_hanger(50)
                              .build();
  const double carrier_initial_mass = em.peek_ship(carrier_id)->mass();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_size(10)
                              .with_hanger(0)
                              .with_max_hanger(0)
                              .build();
  const double fighter_mass = em.peek_ship(fighter_id)->mass();

  em.clear_cache();

  auto result = em.dock_carrier(fighter_id, carrier_id);
  test::expect_true(result.has_value());

  em.clear_cache();
  const auto* carrier_after = em.peek_ship(carrier_id);
  const auto* fighter_after = em.peek_ship(fighter_id);
  test::expect_ne(carrier_after, nullptr);
  test::expect_ne(fighter_after, nullptr);

  test::expect_eq(carrier_after->hanger(), 10);
  test::expect_eq(carrier_after->mass(), carrier_initial_mass + fighter_mass);
  test::expect_eq(fighter_after->dock_state(), DockState::Docked);
  test::expect_true(fighter_after->is_docked());
  test::expect_eq(fighter_after->whatorbits(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(fighter_after->destshipno(), carrier_id);
  test::expect_true(fighter_after->carrier_id().has_value());
  test::expect_eq(*fighter_after->carrier_id(), carrier_id);
}

void test_dock_carrier_errors_and_nested_hierarchy() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& em = ctx.em;

  const auto supercarrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                                   .owned_by(1)
                                   .with_alive(true)
                                   .in_star_orbit(1)
                                   .with_hanger(0)
                                   .with_max_hanger(100)
                                   .build();
  const double supercarrier_base_mass = em.peek_ship(supercarrier_id)->mass();

  const auto subcarrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                                 .owned_by(1)
                                 .with_alive(true)
                                 .in_star_orbit(1)
                                 .with_size(15)
                                 .with_hanger(0)
                                 .with_max_hanger(10)
                                 .build();
  const double subcarrier_base_mass = em.peek_ship(subcarrier_id)->mass();

  const auto fighter1_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                               .owned_by(1)
                               .with_alive(true)
                               .in_star_orbit(1)
                               .with_size(5)
                               .with_hanger(0)
                               .with_max_hanger(0)
                               .build();
  const double fighter1_mass = em.peek_ship(fighter1_id)->mass();

  const auto fighter2_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                               .owned_by(1)
                               .with_alive(true)
                               .in_star_orbit(1)
                               .with_size(4)
                               .with_hanger(0)
                               .with_max_hanger(0)
                               .build();
  const double fighter2_mass = em.peek_ship(fighter2_id)->mass();

  // 1. Self docking rejected
  auto self_res = em.dock_carrier(supercarrier_id, supercarrier_id);
  test::expect_false(self_res.has_value());
  test::expect_eq(self_res.error(), DockError::SelfDocking);

  // 2. Carrier full rejected
  em.mutate_ship(subcarrier_id, [](Ship& c) { c.hanger() = 8; });
  auto full_res = em.dock_carrier(fighter1_id, subcarrier_id);
  test::expect_false(full_res.has_value());
  test::expect_eq(full_res.error(), DockError::CarrierFull);
  em.mutate_ship(subcarrier_id, [](Ship& c) { c.hanger() = 0; });

  // 3. Multi-tier carrier nesting:
  // Dock Fighter 1 into Subcarrier (hanger: 0 -> 5)
  auto f1_dock = em.dock_carrier(fighter1_id, subcarrier_id);
  test::expect_true(f1_dock.has_value());
  test::expect_eq(em.peek_ship(subcarrier_id)->hanger(), 5);
  expect_near(em.peek_ship(subcarrier_id)->mass(),
              subcarrier_base_mass + fighter1_mass);

  // Dock Subcarrier (containing Fighter 1) into Supercarrier
  auto sub_dock = em.dock_carrier(subcarrier_id, supercarrier_id);
  test::expect_true(sub_dock.has_value());
  test::expect_eq(em.peek_ship(supercarrier_id)->hanger(), 15);
  expect_near(em.peek_ship(supercarrier_id)->mass(),
              supercarrier_base_mass + subcarrier_base_mass + fighter1_mass);

  // 4. Cycle detection: Supercarrier cannot dock into Subcarrier or Fighter 1
  auto cycle_sub = em.dock_carrier(supercarrier_id, subcarrier_id);
  test::expect_false(cycle_sub.has_value());
  test::expect_eq(cycle_sub.error(), DockError::CycleDetected);

  auto cycle_f1 = em.dock_carrier(supercarrier_id, fighter1_id);
  test::expect_false(cycle_f1.has_value());
  test::expect_eq(cycle_f1.error(), DockError::CycleDetected);

  // 5. Dock Fighter 2 into Subcarrier while Subcarrier is inside Supercarrier!
  // Mass delta must propagate up to Supercarrier.
  auto f2_dock = em.dock_carrier(fighter2_id, subcarrier_id);
  test::expect_true(f2_dock.has_value());
  test::expect_eq(em.peek_ship(subcarrier_id)->hanger(), 9);
  expect_near(em.peek_ship(subcarrier_id)->mass(),
              subcarrier_base_mass + fighter1_mass + fighter2_mass);
  expect_near(em.peek_ship(supercarrier_id)->mass(),
              supercarrier_base_mass + subcarrier_base_mass + fighter1_mass +
                  fighter2_mass);

  // 6. Undock Fighter 2 from Subcarrier: mass reduction propagates up to
  // Supercarrier
  auto f2_undock = em.undock_carrier(fighter2_id, ScopeLevel::LEVEL_STAR);
  test::expect_true(f2_undock.has_value());
  test::expect_eq(em.peek_ship(subcarrier_id)->hanger(), 5);
  expect_near(em.peek_ship(subcarrier_id)->mass(),
              subcarrier_base_mass + fighter1_mass);
  expect_near(em.peek_ship(supercarrier_id)->mass(),
              supercarrier_base_mass + subcarrier_base_mass + fighter1_mass);

  // 7. Undock Subcarrier from Supercarrier: restores Supercarrier mass &
  // hangar, while Subcarrier still contains Fighter 1.
  auto sub_undock = em.undock_carrier(subcarrier_id, ScopeLevel::LEVEL_STAR);
  test::expect_true(sub_undock.has_value());
  test::expect_eq(em.peek_ship(supercarrier_id)->hanger(), 0);
  expect_near(em.peek_ship(supercarrier_id)->mass(), supercarrier_base_mass);
  test::expect_eq(em.peek_ship(subcarrier_id)->hanger(), 5);
  expect_near(em.peek_ship(subcarrier_id)->mass(),
              subcarrier_base_mass + fighter1_mass);
}

void test_undock_carrier() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& em = ctx.em;

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_hanger(0)
                              .with_max_hanger(50)
                              .build();
  const double carrier_initial_mass = em.peek_ship(carrier_id)->mass();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_size(10)
                              .with_hanger(0)
                              .with_max_hanger(0)
                              .build();

  // Dock first
  auto dock_res = em.dock_carrier(fighter_id, carrier_id);
  test::expect_true(dock_res.has_value());

  // Undock
  auto undock_res = em.undock_carrier(fighter_id, ScopeLevel::LEVEL_STAR);
  test::expect_true(undock_res.has_value());

  em.clear_cache();
  const auto* carrier_after = em.peek_ship(carrier_id);
  const auto* fighter_after = em.peek_ship(fighter_id);

  test::expect_eq(carrier_after->hanger(), 0);
  test::expect_eq(carrier_after->mass(), carrier_initial_mass);
  test::expect_eq(fighter_after->dock_state(), DockState::Spaceborne);
  test::expect_true(fighter_after->is_spaceborne());
  test::expect_false(fighter_after->is_docked());
  test::expect_eq(fighter_after->destshipno(), std::nullopt);
  test::expect_eq(fighter_after->whatorbits(), ScopeLevel::LEVEL_STAR);

  // Undocking an already undocked ship fails
  auto fail_res = em.undock_carrier(fighter_id);
  test::expect_false(fail_res.has_value());
  test::expect_eq(fail_res.error(), UndockError::NotDocked);
}

void test_moor_and_unmoor_ships() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& em = ctx.em;

  const auto s1_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                         .owned_by(1)
                         .with_alive(true)
                         .in_star_orbit(1)
                         .build();
  const auto s2_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                         .owned_by(1)
                         .with_alive(true)
                         .in_star_orbit(1)
                         .build();

  // Self mooring rejected
  auto self_res = em.moor_ships(s1_id, s1_id);
  test::expect_false(self_res.has_value());
  test::expect_eq(self_res.error(), DockError::SelfDocking);

  // Scope mismatch rejected
  em.mutate_ship(s2_id, [](Ship& s) { s.enter_deep_space(); });
  auto scope_res = em.moor_ships(s1_id, s2_id);
  test::expect_false(scope_res.has_value());
  test::expect_eq(scope_res.error(), DockError::ScopeMismatch);
  em.mutate_ship(s2_id, [](Ship& s) { s.enter_star_orbit(1); });

  // Successful mooring
  auto moor_res = em.moor_ships(s1_id, s2_id);
  test::expect_true(moor_res.has_value());

  em.clear_cache();
  const auto* s1_peek = em.peek_ship(s1_id);
  const auto* s2_peek = em.peek_ship(s2_id);

  test::expect_eq(s1_peek->dock_state(), DockState::Docked);
  test::expect_eq(s2_peek->dock_state(), DockState::Docked);
  test::expect_eq(s1_peek->whatorbits(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(s2_peek->whatorbits(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(s1_peek->destshipno(), s2_id);
  test::expect_eq(s2_peek->destshipno(), s1_id);
  test::expect_true(s1_peek->moored_ship_id().has_value());
  test::expect_eq(*s1_peek->moored_ship_id(), s2_id);
  test::expect_false(s1_peek->carrier_id().has_value());

  // Unmoor from s1 clears both
  auto unmoor_res = em.unmoor_ships(s1_id);
  test::expect_true(unmoor_res.has_value());

  em.clear_cache();
  const auto* s1_after = em.peek_ship(s1_id);
  const auto* s2_after = em.peek_ship(s2_id);

  test::expect_eq(s1_after->dock_state(), DockState::Spaceborne);
  test::expect_eq(s2_after->dock_state(), DockState::Spaceborne);
  test::expect_eq(s1_after->destshipno(), std::nullopt);
  test::expect_eq(s2_after->destshipno(), std::nullopt);
}

void test_kill_ship_carrier_and_child_accounting() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& em = ctx.em;

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_hanger(0)
                              .with_max_hanger(50)
                              .build();
  const double carrier_initial_mass = em.peek_ship(carrier_id)->mass();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_size(10)
                              .with_hanger(0)
                              .with_max_hanger(0)
                              .build();

  // Dock fighter into carrier
  auto dock_res = em.dock_carrier(fighter_id, carrier_id);
  test::expect_true(dock_res.has_value());

  // Retiring the child ship directly should update the living carrier's hangar
  // and mass
  em.mutate_ship(fighter_id, [&](Ship& f) { em.destroy_ship(f); });

  em.clear_cache();
  const auto* carrier_after = em.peek_ship(carrier_id);
  test::expect_ne(carrier_after, nullptr);
  test::expect_eq(carrier_after->hanger(), 0);
  test::expect_eq(carrier_after->mass(), carrier_initial_mass);

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(fighter_id); });

  // 3-tier intermediate carrier retirement: destroying Subcarrier while
  // Supercarrier (+ sibling escort) survives must not double-subtract nested
  // craft mass from Supercarrier
  const auto super_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                            .owned_by(1)
                            .with_alive(true)
                            .in_star_orbit(1)
                            .with_max_hanger(50)
                            .build();
  const double super_base_mass = em.peek_ship(super_id)->mass();
  const auto sibling_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_size(10)
                              .build();
  const double sibling_mass = em.peek_ship(sibling_id)->mass();
  const auto sub_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                          .owned_by(1)
                          .with_alive(true)
                          .in_star_orbit(1)
                          .with_size(15)
                          .with_max_hanger(10)
                          .build();
  const auto craft_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                            .owned_by(1)
                            .with_alive(true)
                            .in_star_orbit(1)
                            .with_size(5)
                            .build();

  test::expect_true(em.dock_carrier(sibling_id, super_id).has_value());
  test::expect_true(em.dock_carrier(craft_id, sub_id).has_value());
  test::expect_true(em.dock_carrier(sub_id, super_id).has_value());

  em.mutate_ship(sub_id, [&](Ship& sub) { em.destroy_ship(sub); });

  em.clear_cache();
  const auto* super_after_sub = em.peek_ship(super_id);
  test::expect_eq(super_after_sub->hanger(), 10);
  test::expect_lt(
      std::abs(super_after_sub->mass() - (super_base_mass + sibling_mass)),
      1e-6);
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(sub_id); });
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(craft_id); });

  // Retiring Supercarrier destroys remaining docked craft recursively
  em.mutate_ship(super_id, [&](Ship& s) { em.destroy_ship(s); });

  em.clear_cache();
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(super_id); });
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(sibling_id); });
}

void test_ancestor_carriers_and_corrupt_db_cycles() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& em = ctx.em;

  const auto carrier1_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                               .owned_by(1)
                               .with_alive(true)
                               .in_star_orbit(1)
                               .with_max_hanger(100)
                               .build();
  const auto carrier2_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                               .owned_by(1)
                               .with_alive(true)
                               .in_star_orbit(1)
                               .with_size(20)
                               .with_max_hanger(50)
                               .build();
  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(1)
                              .with_size(10)
                              .build();

  // Multi-tier docking: fighter inside carrier2 inside carrier1
  test::expect_true(em.dock_carrier(fighter_id, carrier2_id).has_value());
  test::expect_true(em.dock_carrier(carrier2_id, carrier1_id).has_value());

  // Chain order verification
  const auto fighter_ancestors = em.ancestor_carriers(fighter_id);
  test::expect_eq(fighter_ancestors.size(), 2zu);
  test::expect_eq(fighter_ancestors[0], carrier2_id);
  test::expect_eq(fighter_ancestors[1], carrier1_id);

  const auto carrier2_ancestors = em.ancestor_carriers(carrier2_id);
  test::expect_eq(carrier2_ancestors.size(), 1zu);
  test::expect_eq(carrier2_ancestors[0], carrier1_id);

  const auto carrier1_ancestors = em.ancestor_carriers(carrier1_id);
  test::expect_true(carrier1_ancestors.empty());

  // Non-existent ship throws EntityNotFoundError
  test::expect_throws<EntityNotFoundError>(
      [&]() { static_cast<void>(em.ancestor_carriers(9999)); });

  // Corrupt-DB cycle detection throws std::runtime_error
  // Force carrier1 to orbit fighter
  em.mutate_ship(carrier1_id,
                 [&](Ship& s) { s.dock_into_carrier(fighter_id); });
  test::expect_throws<std::runtime_error>(
      [&]() { static_cast<void>(em.ancestor_carriers(fighter_id)); });
}

}  // namespace

int main() {
  test_dock_carrier_happy_path();
  test_dock_carrier_errors_and_nested_hierarchy();
  test_undock_carrier();
  test_moor_and_unmoor_ships();
  test_kill_ship_carrier_and_child_accounting();
  test_ancestor_carriers_and_corrupt_db_cycles();
  std::println(std::cout, "All EntityManager dock tests passed!");
  return 0;
}
