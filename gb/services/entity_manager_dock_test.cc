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

void test_dock_carrier_happy_path() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_hanger(0)
                              .with_max_hanger(50)
                              .build();
  const double carrier_initial_mass = em.peek_ship(carrier_id)->mass();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
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

void test_dock_carrier_errors_and_1level_hierarchy() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_hanger(0)
                              .with_max_hanger(30)
                              .build();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_size(10)
                              .with_hanger(0)
                              .with_max_hanger(0)
                              .build();

  // 1. Self docking rejected
  auto self_res = em.dock_carrier(carrier_id, carrier_id);
  test::expect_false(self_res.has_value());
  test::expect_eq(self_res.error(), DockError::SelfDocking);

  // 2. Carrier full rejected
  em.mutate_ship(carrier_id, [](Ship& c) { c.hanger() = 25; });
  auto full_res = em.dock_carrier(fighter_id, carrier_id);
  test::expect_false(full_res.has_value());
  test::expect_eq(full_res.error(), DockError::CarrierFull);
  em.mutate_ship(carrier_id, [](Ship& c) { c.hanger() = 0; });

  // 3. Child contains ships in its hangar (nested carrier rejected)
  const auto subcarrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                                 .owned_by(1)
                                 .with_alive(true)
                                 .in_star_orbit(0)
                                 .with_size(15)
                                 .with_hanger(5)
                                 .with_max_hanger(10)
                                 .build();
  auto nest_res = em.dock_carrier(subcarrier_id, carrier_id);
  test::expect_false(nest_res.has_value());
  test::expect_eq(nest_res.error(), DockError::NestedCarrierDisallowed);

  // 3b. Empty shuttle (max_hanger > 0, hanger == 0) CAN dock into carrier
  const auto shuttle_id = TestShipBuilder(em, ShipType::STYPE_SHUTTLE)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_size(5)
                              .with_hanger(0)
                              .with_max_hanger(2)
                              .build();
  auto shuttle_dock_res = em.dock_carrier(shuttle_id, carrier_id);
  test::expect_true(shuttle_dock_res.has_value());
  auto shuttle_undock_res =
      em.undock_carrier(shuttle_id, ScopeLevel::LEVEL_STAR);
  test::expect_true(shuttle_undock_res.has_value());

  // 4. Carrier itself is docked in another carrier (rejected)
  const auto supercarrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                                   .owned_by(1)
                                   .with_alive(true)
                                   .in_star_orbit(0)
                                   .with_hanger(0)
                                   .with_max_hanger(100)
                                   .build();
  em.mutate_ship(carrier_id,
                 [&](Ship& c) { c.dock_into_carrier(supercarrier_id); });
  auto nested_host_res = em.dock_carrier(fighter_id, carrier_id);
  test::expect_false(nested_host_res.has_value());
  test::expect_eq(nested_host_res.error(), DockError::NestedCarrierDisallowed);
}

void test_undock_carrier() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_hanger(0)
                              .with_max_hanger(50)
                              .build();
  const double carrier_initial_mass = em.peek_ship(carrier_id)->mass();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
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
  test::expect_eq(fighter_after->destshipno(), shipnum_t{0});
  test::expect_eq(fighter_after->whatorbits(), ScopeLevel::LEVEL_STAR);

  // Undocking an already undocked ship fails
  auto fail_res = em.undock_carrier(fighter_id);
  test::expect_false(fail_res.has_value());
  test::expect_eq(fail_res.error(), UndockError::NotDocked);
}

void test_moor_and_unmoor_ships() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  const auto s1_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                         .owned_by(1)
                         .with_alive(true)
                         .in_star_orbit(0)
                         .build();
  const auto s2_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                         .owned_by(1)
                         .with_alive(true)
                         .in_star_orbit(0)
                         .build();

  // Self mooring rejected
  auto self_res = em.moor_ships(s1_id, s1_id);
  test::expect_false(self_res.has_value());
  test::expect_eq(self_res.error(), DockError::SelfDocking);

  // Scope mismatch rejected
  em.mutate_ship(s2_id,
                 [](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_UNIV; });
  auto scope_res = em.moor_ships(s1_id, s2_id);
  test::expect_false(scope_res.has_value());
  test::expect_eq(scope_res.error(), DockError::ScopeMismatch);
  em.mutate_ship(s2_id,
                 [](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_STAR; });

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
  test::expect_eq(s1_after->destshipno(), shipnum_t{0});
  test::expect_eq(s2_after->destshipno(), shipnum_t{0});
}

void test_kill_ship_carrier_and_child_accounting() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = 1;
  race.name = "Tester";
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  const auto carrier_id = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_hanger(0)
                              .with_max_hanger(50)
                              .build();
  const double carrier_initial_mass = em.peek_ship(carrier_id)->mass();

  const auto fighter_id = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .with_size(10)
                              .with_hanger(0)
                              .with_max_hanger(0)
                              .build();

  // Dock fighter into carrier
  auto dock_res = em.dock_carrier(fighter_id, carrier_id);
  test::expect_true(dock_res.has_value());

  // Killing the child ship directly should update the living carrier's hangar
  // and mass
  em.mutate_ship(fighter_id, [&](Ship& f) { em.kill_ship(1, f); });

  em.clear_cache();
  const auto* carrier_after = em.peek_ship(carrier_id);
  test::expect_ne(carrier_after, nullptr);
  test::expect_eq(carrier_after->hanger(), 0);
  test::expect_eq(carrier_after->mass(), carrier_initial_mass);

  const auto* fighter_after = em.peek_ship(fighter_id);
  test::expect_ne(fighter_after, nullptr);
  test::expect_false(fighter_after->alive());
}

}  // namespace

int main() {
  test_dock_carrier_happy_path();
  test_dock_carrier_errors_and_1level_hierarchy();
  test_undock_carrier();
  test_moor_and_unmoor_ships();
  test_kill_ship_carrier_and_child_accounting();
  std::println(std::cout, "All EntityManager dock tests passed!");
  return 0;
}
