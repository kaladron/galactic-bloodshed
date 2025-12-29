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

  // Create EntityManager and JsonStore
  EntityManager em(db);
  JsonStore store(db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "TestRace1";
  race1.governor[0].active = true;
  race1.translate[0] = 100;  // Full knowledge of self
  race1.translate[1] = 50;   // Some knowledge of race 2
  race1.translate[2] = 75;   // Some knowledge of race 3

  Race race2{};
  race2.Playernum = 2;
  race2.name = "TestRace2";
  race2.governor[0].active = true;

  Race race3{};
  race3.Playernum = 3;
  race3.name = "TestRace3";
  race3.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);

  // Create blocks for each player
  // Block 1: Has members but zero VPs (should still appear after fix)
  block block1{};
  block1.Playernum = 1;
  block1.name = "ZeroVPBlock";
  block1.motto = "We have no VPs yet";
  block1.VPs = 0;               // Zero VPs - this was the bug case
  block1.invite = (1ULL << 1);  // Player 1 invited
  block1.pledge = (1ULL << 1);  // Player 1 pledged

  // Block 2: Has members and non-zero VPs
  block block2{};
  block2.Playernum = 2;
  block2.name = "HasVPsBlock";
  block2.motto = "We have some VPs";
  block2.VPs = 100;             // Non-zero VPs
  block2.invite = (1ULL << 2);  // Player 2 invited
  block2.pledge = (1ULL << 2);  // Player 2 pledged

  // Block 3: Empty block (no members, should not appear)
  block block3{};
  block3.Playernum = 3;
  block3.name = "EmptyBlock";
  block3.motto = "Nobody here";
  block3.VPs = 50;
  block3.invite = 0;  // No invites
  block3.pledge = 0;  // No pledges

  BlockRepository blocks(store);
  blocks.save(block1);
  blocks.save(block2);
  blocks.save(block3);

  // Setup Power_blocks global with member counts
  Power_blocks.time = std::time(nullptr);
  Power_blocks.members[0] = 1;  // Block 1 has 1 member
  Power_blocks.members[1] = 1;  // Block 2 has 1 member
  Power_blocks.members[2] = 0;  // Block 3 has 0 members (empty)

  Power_blocks.VPs[0] = 0;    // Block 1: Zero VPs
  Power_blocks.VPs[1] = 100;  // Block 2: Non-zero VPs
  Power_blocks.VPs[2] = 50;   // Block 3: Has VPs but no members

  // Set some other stats for display
  Power_blocks.money[0] = 1000;
  Power_blocks.money[1] = 5000;
  Power_blocks.popn[0] = 10000;
  Power_blocks.popn[1] = 50000;
  Power_blocks.ships_owned[0] = 5;
  Power_blocks.ships_owned[1] = 20;

  // Create GameObj for command execution
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.race = em.peek_race(1);

  std::println("========== Block Command Tests ==========\n");

  std::println("Test 1: List all alliance blocks");
  {
    command_t argv = {"block"};
    GB::commands::block(argv, g);

    std::string output = g.out.str();
    std::println("--- Output ---");
    std::println("{}", output);

    // Verify header doesn't have trailing newline before ===
    // The old bug was: "as of Fri Dec 26...\n ==========" on two lines
    assert(output.find("==========\n #") != std::string::npos ||
           output.find("==========\n Pl") != std::string::npos ||
           output.find("==========\n#") != std::string::npos);
    std::println("    ✓ Header line doesn't have spurious newline");

    // Bug fix verification: Block with zero VPs but members should appear
    assert(output.find("ZeroVPBlock") != std::string::npos &&
           "Bug: Block with zero VPs but members should appear in listing");
    std::println("    ✓ Block with zero VPs but members appears");

    // Block with non-zero VPs should also appear
    assert(output.find("HasVPsBlock") != std::string::npos &&
           "Block with non-zero VPs should appear in listing");
    std::println("    ✓ Block with non-zero VPs appears");

    // Empty block (no members) should NOT appear
    assert(output.find("EmptyBlock") == std::string::npos &&
           "Empty block (no members) should not appear in listing");
    std::println("    ✓ Empty block (no members) correctly hidden");

    g.out.str("");  // Clear for next test
  }

  std::println("\n========== All Tests Passed ==========\n");
  return 0;
}
