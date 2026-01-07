// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // Create in-memory database BEFORE calling initialize_schema()
  Database db(":memory:");

  // Initialize database tables
  initialize_schema(db);

  // Create EntityManager for accessing telegrams
  EntityManager em(db);

  std::println("Running telegram system tests...\n");

  // Test 1: Post telegram using EntityManager
  std::println("Test 1: Post telegram");
  em.post_telegram(1, 0, "First telegram to player 1, governor 0\n");
  em.post_telegram(1, 0, "Second telegram to player 1, governor 0\n");
  em.post_telegram(1, 1, "Telegram to player 1, governor 1\n");
  em.post_telegram(2, 0, "Telegram to player 2, governor 0\n");
  std::println("  ✓ Posted 4 telegrams");

  // Test 2: Check if telegrams exist
  std::println("\nTest 2: Check telegram existence");
  assert(em.has_telegrams(1, 0));
  assert(em.has_telegrams(1, 1));
  assert(em.has_telegrams(2, 0));
  assert(!em.has_telegrams(3, 0));  // No telegrams for player 3
  assert(!em.has_telegrams(1, 2));  // No telegrams for governor 2
  std::println("  ✓ has_telegrams() works correctly");

  // Test 3: Retrieve telegrams
  std::println("\nTest 3: Retrieve telegrams");
  auto telegrams_p1g0 = em.get_telegrams(1, 0);
  assert(telegrams_p1g0.size() == 2);
  assert(telegrams_p1g0[0].message ==
         "First telegram to player 1, governor 0\n");
  assert(telegrams_p1g0[1].message ==
         "Second telegram to player 1, governor 0\n");
  assert(telegrams_p1g0[0].recipient_player == 1);
  assert(telegrams_p1g0[0].recipient_governor == 0);
  std::println("  ✓ Retrieved 2 telegrams for player 1, governor 0");

  auto telegrams_p1g1 = em.get_telegrams(1, 1);
  assert(telegrams_p1g1.size() == 1);
  assert(telegrams_p1g1[0].message == "Telegram to player 1, governor 1\n");
  std::println("  ✓ Retrieved 1 telegram for player 1, governor 1");

  auto telegrams_p2g0 = em.get_telegrams(2, 0);
  assert(telegrams_p2g0.size() == 1);
  assert(telegrams_p2g0[0].message == "Telegram to player 2, governor 0\n");
  std::println("  ✓ Retrieved 1 telegram for player 2, governor 0");

  // Test 4: Verify timestamps are set
  std::println("\nTest 4: Verify timestamps");
  for (const auto& telegram : telegrams_p1g0) {
    assert(telegram.timestamp > 0);
    std::println("  ✓ Telegram ID {} has timestamp {}", telegram.id,
                 telegram.timestamp);
  }

  // Test 5: Delete telegrams for specific governor
  std::println("\nTest 5: Delete telegrams for governor");
  em.delete_telegrams(1, 0);
  assert(!em.has_telegrams(1, 0));
  assert(em.has_telegrams(1, 1));  // Other governor still has telegrams
  assert(em.has_telegrams(2, 0));  // Other player still has telegrams
  std::println("  ✓ Deleted telegrams for player 1, governor 0");

  // Test 6: Verify deletion
  std::println("\nTest 6: Verify deletion");
  auto after_delete = em.get_telegrams(1, 0);
  assert(after_delete.empty());
  std::println("  ✓ No telegrams remain after deletion");

  // Test 7: Test push_telegram() function (high-level API)
  std::println("\nTest 7: Test push_telegram() function");
  push_telegram(em, 3, 0, "Message via push_telegram\n");
  assert(em.has_telegrams(3, 0));
  auto telegrams_p3g0 = em.get_telegrams(3, 0);
  assert(telegrams_p3g0.size() == 1);
  assert(telegrams_p3g0[0].message == "Message via push_telegram\n");
  std::println("  ✓ push_telegram() works correctly");

  // Test 8: Test push_telegram_race() function (sends to all active governors)
  std::println("\nTest 8: Test push_telegram_race() function");

  // Create a race with multiple active governors
  JsonStore store(db);
  RaceRepository race_repo(store);

  Race race4;
  race4.Playernum = 4;
  race4.name = "Test Race";
  race4.Guest = false;
  race4.governor[0].active = true;
  race4.governor[1].active = true;
  race4.governor[2].active = true;
  race_repo.save(race4);

  push_telegram_race(em, 4, "Broadcast to all governors\n");
  assert(em.has_telegrams(4, 0));
  assert(em.has_telegrams(4, 1));
  assert(em.has_telegrams(4, 2));
  auto p4g0 = em.get_telegrams(4, 0);
  auto p4g1 = em.get_telegrams(4, 1);
  auto p4g2 = em.get_telegrams(4, 2);
  assert(p4g0.size() == 1);
  assert(p4g1.size() == 1);
  assert(p4g2.size() == 1);
  assert(p4g0[0].message == "Broadcast to all governors\n");
  assert(p4g1[0].message == "Broadcast to all governors\n");
  assert(p4g2[0].message == "Broadcast to all governors\n");
  std::println("  ✓ push_telegram_race() broadcasts to all active governors");

  // Test 9: Purge all telegrams
  std::println("\nTest 9: Purge all telegrams");
  em.purge_all_telegrams();
  assert(!em.has_telegrams(1, 1));
  assert(!em.has_telegrams(2, 0));
  assert(!em.has_telegrams(3, 0));
  assert(!em.has_telegrams(4, 0));
  assert(!em.has_telegrams(4, 1));
  assert(!em.has_telegrams(4, 2));
  std::println("  ✓ All telegrams purged successfully");

  // Test 10: Test with special characters and long messages
  std::println("\nTest 10: Test special characters and long messages");
  std::string special_msg = "Special chars: <>|&;$`\"'\\n\ttest\n";
  em.post_telegram(5, 0, special_msg);
  auto special_telegrams = em.get_telegrams(5, 0);
  assert(special_telegrams.size() == 1);
  assert(special_telegrams[0].message == special_msg);
  std::println("  ✓ Special characters handled correctly");

  std::string long_msg(1000, 'X');
  long_msg += "\n";
  em.post_telegram(5, 1, long_msg);
  auto long_telegrams = em.get_telegrams(5, 1);
  assert(long_telegrams.size() == 1);
  assert(long_telegrams[0].message == long_msg);
  std::println("  ✓ Long messages (1000+ chars) handled correctly");

  // Test 11: Test empty message handling
  std::println("\nTest 11: Test empty message");
  em.post_telegram(6, 0, "");
  auto empty_telegrams = em.get_telegrams(6, 0);
  assert(empty_telegrams.size() == 1);
  assert(empty_telegrams[0].message == "");
  std::println("  ✓ Empty messages handled correctly");

  // Test 12: Test order preservation (FIFO)
  std::println("\nTest 12: Test FIFO order preservation");
  em.post_telegram(7, 0, "First\n");
  em.post_telegram(7, 0, "Second\n");
  em.post_telegram(7, 0, "Third\n");
  auto ordered_telegrams = em.get_telegrams(7, 0);
  assert(ordered_telegrams.size() == 3);
  assert(ordered_telegrams[0].message == "First\n");
  assert(ordered_telegrams[1].message == "Second\n");
  assert(ordered_telegrams[2].message == "Third\n");
  // Timestamps should be in order (may be equal if added very quickly)
  assert(ordered_telegrams[0].timestamp <= ordered_telegrams[1].timestamp);
  assert(ordered_telegrams[1].timestamp <= ordered_telegrams[2].timestamp);
  std::println("  ✓ Telegrams retrieved in FIFO order");

  // Test 13: Test multiple recipients don't interfere
  std::println("\nTest 13: Test recipient isolation");
  em.post_telegram(8, 0, "Player 8, Gov 0\n");
  em.post_telegram(8, 1, "Player 8, Gov 1\n");
  em.post_telegram(9, 0, "Player 9, Gov 0\n");

  auto p8g0_telegrams = em.get_telegrams(8, 0);
  auto p8g1_telegrams = em.get_telegrams(8, 1);
  auto p9g0_telegrams = em.get_telegrams(9, 0);

  assert(p8g0_telegrams.size() == 1);
  assert(p8g1_telegrams.size() == 1);
  assert(p9g0_telegrams.size() == 1);
  assert(p8g0_telegrams[0].message == "Player 8, Gov 0\n");
  assert(p8g1_telegrams[0].message == "Player 8, Gov 1\n");
  assert(p9g0_telegrams[0].message == "Player 9, Gov 0\n");
  std::println("  ✓ Recipient isolation works correctly");

  // Test 14: Delete one recipient's telegrams doesn't affect others
  std::println("\nTest 14: Test selective deletion");
  em.delete_telegrams(8, 0);
  assert(!em.has_telegrams(8, 0));
  assert(em.has_telegrams(8, 1));  // Different governor still has telegrams
  assert(em.has_telegrams(9, 0));  // Different player still has telegrams
  std::println("  ✓ Selective deletion doesn't affect other recipients");

  std::println("\n✅ All telegram system tests passed!");
  return 0;
}
