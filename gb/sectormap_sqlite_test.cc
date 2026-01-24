// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import std;

#include <cassert>

// Helper to populate a sector map with test data
void populate_sectormap(SectorMap& smap, const Planet& planet, int base_eff,
                        int base_popn) {
  for (int y = 0; y < planet.Maxy(); y++) {
    for (int x = 0; x < planet.Maxx(); x++) {
      auto& sector = smap.get(x, y);
      sector.set_x(x);
      sector.set_y(y);
      sector.set_eff(base_eff + (x * y));
      sector.set_fert(30 + x);
      sector.set_mobilization(10 + y);
      sector.set_crystals(x * 10);
      sector.set_resource(y * 20);
      sector.set_popn_exact(base_popn * (x + 1) * (y + 1));
      sector.set_troops(x + y);
      sector.set_owner((x + y) % 2 + 1);
      sector.set_race(sector.get_owner());

      // Vary sector types
      if (x == 0 || x == planet.Maxx() - 1 || y == 0 ||
          y == planet.Maxy() - 1) {
        sector.set_type(SectorType::SEC_SEA);
        sector.set_condition(SectorType::SEC_SEA);
      } else if ((x + y) % 3 == 0) {
        sector.set_type(SectorType::SEC_MOUNT);
        sector.set_condition(SectorType::SEC_MOUNT);
      } else if ((x + y) % 3 == 1) {
        sector.set_type(SectorType::SEC_LAND);
        sector.set_condition(SectorType::SEC_LAND);
      } else {
        sector.set_type(SectorType::SEC_FOREST);
        sector.set_condition(SectorType::SEC_FOREST);
      }
    }
  }
}

// Helper to verify two sector maps are identical
void verify_sectormap_equal(const SectorMap& original,
                            const SectorMap& retrieved, const Planet& planet) {
  for (int y = 0; y < planet.Maxy(); y++) {
    for (int x = 0; x < planet.Maxx(); x++) {
      const auto& orig = original.get(x, y);
      const auto& retr = retrieved.get(x, y);

      assert(retr.get_x() == orig.get_x());
      assert(retr.get_y() == orig.get_y());
      assert(retr.get_eff() == orig.get_eff());
      assert(retr.get_fert() == orig.get_fert());
      assert(retr.get_mobilization() == orig.get_mobilization());
      assert(retr.get_crystals() == orig.get_crystals());
      assert(retr.get_resource() == orig.get_resource());
      assert(retr.get_popn() == orig.get_popn());
      assert(retr.get_troops() == orig.get_troops());
      assert(retr.get_owner() == orig.get_owner());
      assert(retr.get_race() == orig.get_race());
      assert(retr.get_type() == orig.get_type());
      assert(retr.get_condition() == orig.get_condition());
    }
  }
}

void test_entitymanager_sectormap(EntityManager& em, Database& db) {
  std::println("=== Testing EntityManager get_sectormap/peek_sectormap ===");

  // First, we need to create and save a planet so EntityManager can find it
  JsonStore store(db);
  PlanetRepository planets(store);

  Planet test_planet{PlanetType::WATER};
  test_planet.star_id() = 5;
  test_planet.planet_order() = 1;
  test_planet.Maxx() = 8;
  test_planet.Maxy() = 6;
  planets.save(test_planet);

  // Create initial sector data using Repository (DAL layer)
  SectorMap initial_smap(test_planet, true);
  populate_sectormap(initial_smap, test_planet, 25, 50);
  SectorRepository sectors(store);
  sectors.save_map(initial_smap);

  // Test get_sectormap with RAII handle
  {
    auto smap_handle = em.get_sectormap(5, 1);
    assert(smap_handle.get() && "get_sectormap should return valid handle");

    auto& smap = *smap_handle;

    // Verify data was loaded correctly
    assert(smap.get(0, 0).get_eff() == 25);  // base_eff + 0*0 = 25

    // Modify some sectors
    for (int i = 0; i < 4; i++) {
      auto& sector = smap.get(i, i);
      sector.set_eff(95);
      sector.set_popn_exact(77777);
    }
    // Handle auto-saves when going out of scope
  }

  std::println("  get_sectormap with RAII auto-save: PASSED");

  // Clear cache to force reload from DB
  em.clear_cache();

  // Verify the updates persisted by loading again
  {
    auto smap_handle = em.get_sectormap(5, 1);
    assert(smap_handle.get());
    const auto& smap = smap_handle.read();

    for (int i = 0; i < 4; i++) {
      assert(smap.get(i, i).get_eff() == 95);
      assert(smap.get(i, i).get_popn() == 77777);
    }

    // Verify other sectors unchanged
    assert(smap.get(5, 4).get_eff() == 25 + 5 * 4);  // base_eff + x*y
  }

  std::println("  Update persistence verified: PASSED");

  // Test peek_sectormap (read-only)
  {
    const SectorMap* smap_ptr = em.peek_sectormap(5, 1);
    assert(smap_ptr && "peek_sectormap should return valid pointer");

    // Verify we see the updated data
    assert(smap_ptr->get(0, 0).get_eff() == 95);
  }

  std::println("  peek_sectormap read-only access: PASSED");

  // Test caching - multiple handles should reference same data
  {
    auto handle1 = em.get_sectormap(5, 1);
    auto handle2 = em.get_sectormap(5, 1);

    assert(handle1.get() == handle2.get() &&
           "Multiple handles should reference same cached data");

    // Modify via handle1
    (*handle1).get(7, 5).set_eff(42);

    // Should see change via handle2 (same underlying object)
    assert((*handle2).get(7, 5).get_eff() == 42);
  }

  std::println("  Caching (single instance) verified: PASSED");

  // Test with non-existent planet
  {
    auto smap_handle = em.get_sectormap(999, 999);
    assert(!smap_handle.get() &&
           "Non-existent planet should return null handle");
  }

  std::println("  Non-existent planet handling: PASSED");
}

void test_multiple_planets_isolation(EntityManager& em, Database& db) {
  std::println("=== Testing multiple planets isolation ===");

  // Create and save planets so EntityManager can find them
  JsonStore store(db);
  PlanetRepository planets(store);

  Planet planet1{PlanetType::EARTH};
  planet1.star_id() = 7;
  planet1.planet_order() = 0;
  planet1.Maxx() = 6;
  planet1.Maxy() = 6;
  planets.save(planet1);

  Planet planet2{PlanetType::WATER};
  planet2.star_id() = 7;       // Same star
  planet2.planet_order() = 1;  // Different planet order
  planet2.Maxx() = 4;
  planet2.Maxy() = 4;
  planets.save(planet2);

  // Create different sector maps for each planet using Repository
  SectorMap smap1(planet1, true);
  populate_sectormap(smap1, planet1, 10, 100);
  SectorRepository sectors(store);
  sectors.save_map(smap1);

  SectorMap smap2(planet2, true);
  for (int y = 0; y < planet2.Maxy(); y++) {
    for (int x = 0; x < planet2.Maxx(); x++) {
      auto& sector = smap2.get(x, y);
      sector.set_x(x);
      sector.set_y(y);
      sector.set_eff(99);
      sector.set_popn_exact(12345);
      sector.set_type(SectorType::SEC_SEA);
      sector.set_condition(SectorType::SEC_SEA);
    }
  }
  sectors.save_map(smap2);

  // Load both via EntityManager and verify they're different
  const SectorMap* reload1 = em.peek_sectormap(7, 0);
  const SectorMap* reload2 = em.peek_sectormap(7, 1);

  assert(reload1 && reload2);

  // Planet 1 should have its values (base_eff=10, so (0,0) = 10 + 0*0 = 10)
  assert(reload1->get(0, 0).get_eff() == 10);
  assert(reload1->get(0, 0).get_popn() == 100);  // base_popn * 1 * 1

  // Planet 2 should have its own distinct values
  assert(reload2->get(0, 0).get_eff() == 99);
  assert(reload2->get(0, 0).get_popn() == 12345);

  std::println("  Planet isolation: PASSED");
}

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling
  // initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  // Create EntityManager for EntityManager-based tests
  EntityManager em(db);

  // Run all tests
  test_entitymanager_sectormap(em, db);
  test_multiple_planets_isolation(em, db);

  std::println("\nAll SectorMap tests passed!");
  return 0;
}
