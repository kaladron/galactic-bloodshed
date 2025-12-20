// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);
  
  // Create EntityManager
  EntityManager em(db);
  JsonStore store(db);
  
  // Create two test races (sender and receiver)
  Race sender{};
  sender.Playernum = 1;
  sender.Guest = false;
  sender.Gov_ship = 0;
  sender.governor[0].name = "TestGovernor";
  sender.name = "TestRace";
  sender.translate[0] = 50;  // Initial translation with race 2
  
  Race receiver{};
  receiver.Playernum = 2;
  receiver.Guest = false;
  receiver.Gov_ship = 0;
  receiver.God = false;
  receiver.name = "AlienRace";
  
  RaceRepository races(store);
  races.save(sender);
  races.save(receiver);
  
  // Create star
  star_struct star{};
  star.star_id = 0;
  star.AP[0] = 10;  // Sender has APs
  
  StarRepository stars(store);
  stars.save(star);
  
  // Create GameObj for sender
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_STAR;
  g.snum = 0;
  g.god = false;
  
  // Test sending a regular message: send_message 2 Hello World
  command_t argv = {"send_message", "2", "Hello", "World"};
  GB::commands::send_message(argv, g);
  
  // Verify translation modifier increased
  const auto* updated_receiver = em.peek_race(2);
  assert(updated_receiver);
  
  // Translation should have increased by 2 (from 50 to 52)
  assert(updated_receiver->translate[0] == 52);
  
  std::println("✓ send_message command: Message sent and translation modifier persisted");
  return 0;
}
