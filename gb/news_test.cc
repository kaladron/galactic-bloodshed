// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // Create in-memory database BEFORE calling initialize_schema()
  Database db(":memory:");

  // Initialize database tables
  initialize_schema(db);

  // Create EntityManager and repositories
  EntityManager em(db);
  NewsRepository news_repo(db);

  // Test 1: Add news items
  auto id1 = news_repo.add(NewsType::ANNOUNCE, "Server started\n");
  assert(id1.has_value());
  std::println("Added announcement with ID: {}", *id1);

  auto id2 = news_repo.add(NewsType::COMBAT, "Battle between races!\n");
  assert(id2.has_value());
  std::println("Added combat news with ID: {}", *id2);

  auto id3 = news_repo.add(NewsType::DECLARATION, "Alliance formed\n");
  assert(id3.has_value());
  std::println("Added declaration with ID: {}", *id3);

  // Test 2: Retrieve news items
  auto announce_items = news_repo.get_since(NewsType::ANNOUNCE, 0);
  assert(announce_items.size() == 1);
  assert(announce_items[0].message == "Server started\n");
  std::println("Retrieved {} announcement(s)", announce_items.size());

  auto combat_items = news_repo.get_since(NewsType::COMBAT, 0);
  assert(combat_items.size() == 1);
  assert(combat_items[0].message == "Battle between races!\n");
  std::println("Retrieved {} combat news item(s)", combat_items.size());

  // Test 3: Get latest ID
  int latest_announce_id = news_repo.get_latest_id(NewsType::ANNOUNCE);
  assert(latest_announce_id == *id1);
  std::println("Latest announcement ID: {}", latest_announce_id);

  // Test 4: Pagination - add more items and test get_since
  auto id4 = news_repo.add(NewsType::ANNOUNCE, "Update completed\n");
  assert(id4.has_value());

  auto new_announce_items = news_repo.get_since(NewsType::ANNOUNCE, *id1);
  assert(new_announce_items.size() == 1);
  assert(new_announce_items[0].message == "Update completed\n");
  std::println("Retrieved {} new announcement(s) since ID {}",
               new_announce_items.size(), *id1);

  // Test 5: Purge specific type
  bool purged = news_repo.purge_type(NewsType::COMBAT);
  assert(purged);
  auto combat_after_purge = news_repo.get_since(NewsType::COMBAT, 0);
  assert(combat_after_purge.empty());
  std::println("Combat news purged successfully");

  // Test 6: Purge all
  bool purged_all = news_repo.purge_all();
  assert(purged_all);

  auto all_announce = news_repo.get_since(NewsType::ANNOUNCE, 0);
  auto all_declaration = news_repo.get_since(NewsType::DECLARATION, 0);
  assert(all_announce.empty());
  assert(all_declaration.empty());
  std::println("All news purged successfully");

  // Test 7: Test post() function (high-level API)
  post(em, "Test message;with special|chars", NewsType::ANNOUNCE);
  auto final_items = news_repo.get_since(NewsType::ANNOUNCE, 0);
  assert(final_items.size() == 1);
  // Check that special chars were replaced (';' -> '\n', '|' -> '\t')
  assert(final_items[0].message.find('\n') != std::string::npos);
  assert(final_items[0].message.find('\t') != std::string::npos);
  std::println("Special character replacement working correctly");

  std::println("\nAll news repository tests passed! ✅");
  return 0;
}
