// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

using playernum_t = ID<"player", int>;
using shipnum_t = ID<"ship", int>;
using starnum_t = ID<"star", int>;

int main() {
  // Test basic construction
  playernum_t player1{1};
  playernum_t player2{2};
  
  assert(player1.value == 1);
  assert(player2.value == 2);
  
  // Test comparison
  assert(player1 != player2);
  assert(player1 < player2);
  assert(player2 > player1);
  
  // Test type safety - these types are distinct
  shipnum_t ship{42};
  starnum_t star{5};
  
  assert(ship.value == 42);
  assert(star.value == 5);
  
  // Test increment/decrement
  playernum_t p{10};
  ++p;
  assert(p.value == 11);
  p++;
  assert(p.value == 12);
  --p;
  assert(p.value == 11);
  
  // Test dereferencing
  assert(*p == 11);
  
  // Test formatting
  std::string output = std::format("Player: {}, Ship: {}, Star: {}\n", 
                                   player1, ship, star);
  assert(!output.empty());
  std::println("{}", output);
  
  // Test hash support (for use in unordered containers)
  std::unordered_map<playernum_t, std::string> player_names;
  player_names[player1] = "Alice";
  player_names[player2] = "Bob";
  
  assert(player_names[player1] == "Alice");
  assert(player_names[player2] == "Bob");
  
  std::println("All strong_id tests passed!");
  return 0;
}
