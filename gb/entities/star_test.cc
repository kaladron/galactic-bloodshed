// SPDX-License-Identifier: Apache-2.0

/// \file star_test.cc
/// \brief Unit tests for Star class methods, planet name manipulation,
/// auto-resizing, and bounds checking.

import dallib;
import gb.entities;
import test;
import std;

int main() {
  // Direct Star constructor and primary-key validation
  std::println(std::cout, "Direct Star constructor and PK validation...");
  {
    Star named_star{3, "Sirius", {150.0, -250.0}};
    test::expect_eq(named_star.star_id(), starnum_t{3});
    test::expect_eq(named_star.get_name(), "Sirius");
    test::expect_eq(named_star.coordinates(),
                    UniverseCoordinates{150.0, -250.0});
    test::expect_eq(named_star.numplanets(), 0);

    // Constructing a Star with star_id == 0 or uninitialized star_struct throws
    test::expect_throws<std::invalid_argument>(
        []() { (void)Star{starnum_t{0}, "Invalid"}; });
    test::expect_throws<std::invalid_argument>(
        []() { (void)Star{star_struct{.name = "MissingId"}}; });
    std::println(std::cout, "  ✓ Direct constructor and PK validation work");
  }

  // Basic star creation with vector of planet names
  std::println(std::cout, "Basic star creation with planet names...");
  {
    Star star(star_struct{
        .name = "Sol",
        .pnames = {"Mercury", "Venus", "Earth"},
        .star_id = 1,
    });

    test::expect_eq(star.get_name(), "Sol");
    test::expect_eq(star.numplanets(), 3);
    test::expect_eq(star.get_planet_name(1), "Mercury");
    test::expect_eq(star.get_planet_name(2), "Venus");
    test::expect_eq(star.get_planet_name(3), "Earth");
    std::println(std::cout, "  ✓ Basic creation and access works");
  }

  // Bounds checking on get_planet_name (out of range throws exception)
  std::println(std::cout, "Bounds checking on get_planet_name...");
  {
    Star star(star_struct{
        .name = "Test",
        .pnames = {"Planet1", "Planet2"},
        .star_id = 1,
    });

    // Valid access
    test::expect_eq(star.get_planet_name(1), "Planet1");
    test::expect_eq(star.get_planet_name(2), "Planet2");

    // Out of bounds - should throw exception
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.get_planet_name(0); });
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.get_planet_name(3); });
    std::println(std::cout, "  ✓ Out of bounds access throws exception");
  }

  // planet_name_isset bounds checking (throws on out of bounds)
  std::println(std::cout, "planet_name_isset bounds checking...");
  {
    Star star(star_struct{
        .name = "Test",
        .pnames = {"Planet1", "", "Planet3"},
        .star_id = 1,
    });

    test::expect_true(star.planet_name_isset(1));   // Has name
    test::expect_false(star.planet_name_isset(2));  // Empty name
    test::expect_true(star.planet_name_isset(3));   // Has name

    // Out of bounds - should throw exception
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.planet_name_isset(0); });
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.planet_name_isset(99); });
    std::println(
        std::cout,
        "  ✓ planet_name_isset works correctly and throws on out of bounds");
  }

  // set_planet_name with auto-resize
  std::println(std::cout, "set_planet_name with auto-resize...");
  {
    Star star{1, "Test"};
    star.set_planet_name(1, "Planet1");
    test::expect_eq(star.numplanets(), 1);

    // Set planet at 1-based index 6 - should auto-resize vector to 6
    star.set_planet_name(6, "Jupiter");
    test::expect_eq(star.numplanets(), 6);

    // Check that intermediate planets exist but are empty
    test::expect_eq(star.get_planet_name(1), "Planet1");
    test::expect_eq(star.get_planet_name(2), "");
    test::expect_eq(star.get_planet_name(3), "");
    test::expect_eq(star.get_planet_name(4), "");
    test::expect_eq(star.get_planet_name(5), "");
    test::expect_eq(star.get_planet_name(6), "Jupiter");
    test::expect_throws<std::runtime_error>(
        [&]() { star.set_planet_name(0, "Invalid"); });
    std::println(std::cout, "  ✓ Auto-resize works correctly");
  }

  // Overwriting existing planet names
  std::println(std::cout, "Overwriting existing planet names...");
  {
    Star star{1, "Test"};
    star.set_planet_name(1, "OldName");
    test::expect_eq(star.get_planet_name(1), "OldName");

    star.set_planet_name(1, "NewName");
    test::expect_eq(star.get_planet_name(1), "NewName");
    test::expect_eq(star.numplanets(), 1);  // Size unchanged
    std::println(std::cout, "  ✓ Overwriting works correctly");
  }

  // Empty star (no planets, bounds checking throws)
  std::println(std::cout, "Empty star (no planets)...");
  {
    Star star{1, "EmptyStar"};
    test::expect_eq(star.numplanets(), 0);

    // Out of bounds access should throw
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.get_planet_name(1); });

    // planet_name_isset should also throw
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.planet_name_isset(1); });

    std::println(
        std::cout,
        "  ✓ Empty star works correctly with exception-based bounds checking");
  }

  // numplanets() reflects vector size
  std::println(std::cout, "numplanets() reflects vector size...");
  {
    Star star{1, "Test"};
    test::expect_eq(star.numplanets(), 0);

    // Construct from designated initializer
    Star star2(star_struct{
        .name = "Test",
        .pnames = {"P1", "P2", "P3"},
        .star_id = 1,
    });
    test::expect_eq(star2.numplanets(), 3);

    // Modify through Star interface
    star2.set_planet_name(4, "P4");
    test::expect_eq(star2.numplanets(), 4);
    std::println(std::cout, "  ✓ numplanets() correctly reflects vector size");
  }

  // Star::control tests
  std::println(std::cout, "Star::control administrative authorization...");
  {
    Star star{1, "SectorGovStar"};

    // Governor 1 (primary race leader) always has administrative control
    test::expect_true(star.control(1, 1));
    test::expect_true(star.control(2, 1));

    // Default governor is 1, so any subordinate governor query for non-assigned
    // player fails
    test::expect_false(star.control(1, 2));
    test::expect_false(star.control(1, 3));

    // Assign specific governor for player 1
    star.set_governor(1, 3);
    test::expect_true(star.control(1, 1));   // Primary leader still controls
    test::expect_true(star.control(1, 3));   // Assigned governor has control
    test::expect_false(star.control(1, 2));  // Other governors do not

    // Player 2's assignment is isolated
    star.set_governor(2, 4);
    test::expect_true(star.control(2, 4));
    test::expect_false(star.control(2, 3));
    test::expect_false(star.control(1, 4));
    std::println(std::cout, "  ✓ Star::control correctly authorizes governors");
  }

  // Exploration domain methods
  std::println(std::cout, "Star exploration domain methods...");
  {
    Star star{1, "Alpha"};

    test::expect_false(star.is_explored());
    test::expect_false(star.is_explored_by(player_t{1}));
    test::expect_false(star.is_explored_by(player_t{2}));

    star.mark_explored_by(player_t{1});
    test::expect_true(star.is_explored());
    test::expect_true(star.is_explored_by(player_t{1}));
    test::expect_false(star.is_explored_by(player_t{2}));

    star.mark_explored_by(player_t{2});
    test::expect_true(star.is_explored());
    test::expect_true(star.is_explored_by(player_t{1}));
    test::expect_true(star.is_explored_by(player_t{2}));

    star.clear_explored_by(player_t{1});
    test::expect_false(star.is_explored_by(player_t{1}));
    test::expect_true(star.is_explored_by(player_t{2}));
    test::expect_true(star.is_explored());

    star.clear_all_explored();
    test::expect_false(star.is_explored());
    test::expect_false(star.is_explored_by(player_t{2}));
    std::println(std::cout, "  ✓ Star exploration methods work as expected");
  }

  // Inhabitation domain methods
  std::println(std::cout, "Star inhabitation domain methods...");
  {
    Star star{1, "Beta"};

    test::expect_false(star.is_inhabited());
    test::expect_false(star.is_inhabited_by(player_t{1}));
    test::expect_false(star.is_inhabited_by(player_t{2}));

    star.mark_inhabited_by(player_t{1});
    test::expect_true(star.is_inhabited());
    test::expect_true(star.is_inhabited_by(player_t{1}));
    test::expect_false(star.is_inhabited_by(player_t{2}));

    star.mark_inhabited_by(player_t{2});
    test::expect_true(star.is_inhabited_by(player_t{2}));

    star.clear_inhabited_by(player_t{1});
    test::expect_false(star.is_inhabited_by(player_t{1}));
    test::expect_true(star.is_inhabited_by(player_t{2}));
    test::expect_true(star.is_inhabited());

    star.clear_all_inhabitants();
    test::expect_false(star.is_inhabited());
    test::expect_false(star.is_inhabited_by(player_t{2}));

    // Verify std::flat_set<player_t> direct accessor
    star.inhabited().insert(player_t{3});
    test::expect_true(star.is_inhabited_by(player_t{3}));
    test::expect_true(star.inhabited().contains(player_t{3}));
    test::expect_eq(star.inhabited().size(), 1zu);

    std::println(std::cout, "  ✓ Star inhabitation methods work as expected");
  }

  // AP, governor, and ground_assaults sparse map tests
  std::println(
      std::cout,
      "Star AP, governor, and ground_assaults sparse map accessors...");
  {
    Star star{1, "Gamma"};
    const Star& cstar = star;

    // Read-only access on unpopulated star returns defaults without mutating
    test::expect_eq(cstar.AP(player_t{1}), 0);
    test::expect_eq(star.governor(player_t{1}), Race::leader_id);
    test::expect_eq(cstar.governor(player_t{1}), Race::leader_id);
    test::expect_eq(cstar.ground_assault_count(player_t{1}, player_t{2}), 0u);
    test::expect_true(cstar.get_struct().AP.empty());
    test::expect_true(cstar.get_struct().governor.empty());
    test::expect_true(cstar.get_struct().ground_assaults.empty());

    star.AP(player_t{1}) = 42;
    star.AP(player_t{2}) = 99;
    test::expect_eq(cstar.AP(player_t{1}), 42);
    test::expect_eq(cstar.AP(player_t{2}), 99);

    star.set_governor(player_t{1}, 3);
    test::expect_eq(cstar.governor(player_t{1}), 3);
    test::expect_eq(cstar.get_struct().governor.size(), 1zu);

    // Resetting governor to Race::leader_id erases the sparse entry
    star.set_governor(player_t{1}, Race::leader_id);
    test::expect_eq(cstar.governor(player_t{1}), Race::leader_id);
    test::expect_true(cstar.get_struct().governor.empty());

    star.record_ground_assault(player_t{1}, player_t{2}, 2);
    test::expect_eq(cstar.ground_assault_count(player_t{1}, player_t{2}), 2u);
    star.clear_ground_assaults(player_t{1}, player_t{2});
    test::expect_eq(cstar.ground_assault_count(player_t{1}, player_t{2}), 0u);

    // Bounds checking throws std::out_of_range on player_t < 1
    test::expect_throws<std::out_of_range>(
        [&]() { (void)star.AP(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)cstar.AP(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)star.governor(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { star.set_governor(player_t{0}, 2); });
    test::expect_throws<std::out_of_range>(
        [&]() { star.record_ground_assault(player_t{0}, player_t{1}); });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)cstar.ground_assault_count(player_t{1}, player_t{0}); });
    std::println(std::cout,
                 "  ✓ Star AP, governor, and ground_assaults verified");
  }

  // get_random_planet_index tests
  std::println(std::cout, "Star get_random_planet_index tests...");
  {
    // Case 1: Single planet system always returns index 1
    Star star1(
        star_struct{.name = "Solo", .pnames = {"SingleWorld"}, .star_id = 1});
    test::expect_eq(star1.get_random_planet_index(), planetnum_t{1});

    // Case 2: Multi-planet system returns a valid index in range [1,
    // numplanets]
    Star star3(star_struct{.name = "Trio",
                           .pnames = {"World1", "World2", "World3"},
                           .star_id = 1});
    for (int i = 0; i < 20; ++i) {
      planetnum_t p = star3.get_random_planet_index();
      test::expect_true(p >= 1 && p <= star3.numplanets());
    }

    std::println(std::cout, "  ✓ get_random_planet_index verified (bounds)");
  }

  // coordinates tests
  std::println(std::cout, "Star coordinates tests...");
  {
    Star star{1, "", {450.0, -850.0}};
    test::expect_eq(star.coordinates(), UniverseCoordinates(450.0, -850.0));
    star.set_coordinates(UniverseCoordinates(-100.0, 200.0));
    test::expect_eq(star.coordinates().x, -100.0);
    test::expect_eq(star.coordinates().y, 200.0);
    test::expect_eq(star.coordinates(), UniverseCoordinates(-100.0, 200.0));
    std::println(std::cout, "  ✓ coordinates() and set_coordinates() verified");
  }

  std::println(std::cout, "\n✓ All Star class tests passed!");
  return 0;
}
