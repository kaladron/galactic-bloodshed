// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 1;

  // Set up resources on planet info for various players
  planet.info(player_t{1}).resource = 0;
  planet.info(player_t{2}).resource = 500;
  planet.info(player_t{3}).resource = 1000;
  planet.info(player_t{4}).resource = 0;

  // 1. Order [1, 2, 3, 4] -> Should pick player 2 (first in order with
  // resources > 0)
  std::vector<player_t> order1 = {player_t{1}, player_t{2}, player_t{3},
                                  player_t{4}};
  auto victim1 = select_victim_to_steal_from(planet, order1);
  assert(victim1.has_value());
  assert(victim1->value == 2);

  // 2. Order [3, 2, 1, 4] -> Should pick player 3 (first in order with
  // resources > 0)
  std::vector<player_t> order2 = {player_t{3}, player_t{2}, player_t{1},
                                  player_t{4}};
  auto victim2 = select_victim_to_steal_from(planet, order2);
  assert(victim2.has_value());
  assert(victim2->value == 3);

  // 3. Order [1, 4] -> None have resources -> Should return std::nullopt
  std::vector<player_t> order3 = {player_t{1}, player_t{4}};
  auto victim3 = select_victim_to_steal_from(planet, order3);
  assert(!victim3.has_value());

  // 4. Test shuffled_indices produces valid race IDs permutation
  auto rand_ids = shuffled_indices(1, 5);  // 1 to 4 inclusive
  assert(rand_ids.size() == 4);
  std::set<int> seen(rand_ids.begin(), rand_ids.end());
  assert(seen.size() == 4);
  assert(seen.contains(1) && seen.contains(2) && seen.contains(3) &&
         seen.contains(4));

  std::println(std::cout, "✓ select_victim_to_steal_from unit test passed!");
  return 0;
}
