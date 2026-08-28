// SPDX-License-Identifier: Apache-2.0

/// \file news_test.cc
/// \brief Unit tests for NewsRepository CRUD, category filtering, paging, and
/// special character sanitization.

import dallib;
import gblib;
import test;
import std;

int main() {
  // Create in-memory database BEFORE calling initialize_schema()
  Database db(":memory:");

  // Initialize database tables
  initialize_schema(db);

  // Create EntityManager and repositories
  EntityManager em(db);
  NewsRepository news_repo(db);

  // Add news items
  auto id1 = news_repo.add(NewsType::ANNOUNCE, "Server started\n");
  test::expect_true(id1.has_value());
  std::println(std::cout, "Added announcement with ID: {}", *id1);

  auto id2 = news_repo.add(NewsType::COMBAT, "Battle between races!\n");
  test::expect_true(id2.has_value());
  std::println(std::cout, "Added combat news with ID: {}", *id2);

  auto id3 = news_repo.add(NewsType::DECLARATION, "Alliance formed\n");
  test::expect_true(id3.has_value());
  std::println(std::cout, "Added declaration with ID: {}", *id3);

  // Retrieve news items
  auto announce_items = news_repo.get_since(NewsType::ANNOUNCE, 0);
  test::expect_eq(announce_items.size(), 1);
  test::expect_eq(announce_items[0].message, "Server started\n");
  std::println(std::cout, "Retrieved {} announcement(s)",
               announce_items.size());

  auto combat_items = news_repo.get_since(NewsType::COMBAT, 0);
  test::expect_eq(combat_items.size(), 1);
  test::expect_eq(combat_items[0].message, "Battle between races!\n");
  std::println(std::cout, "Retrieved {} combat news item(s)",
               combat_items.size());

  // Get latest ID
  int latest_announce_id = news_repo.get_latest_id(NewsType::ANNOUNCE);
  test::expect_eq(latest_announce_id, *id1);
  std::println(std::cout, "Latest announcement ID: {}", latest_announce_id);

  // Pagination - add more items and test get_since
  auto id4 = news_repo.add(NewsType::ANNOUNCE, "Update completed\n");
  test::expect_true(id4.has_value());

  auto new_announce_items = news_repo.get_since(NewsType::ANNOUNCE, *id1);
  test::expect_eq(new_announce_items.size(), 1);
  test::expect_eq(new_announce_items[0].message, "Update completed\n");
  std::println(std::cout, "Retrieved {} new announcement(s) since ID {}",
               new_announce_items.size(), *id1);

  // Purge specific type
  bool purged = news_repo.purge_type(NewsType::COMBAT);
  test::expect_true(purged);
  auto combat_after_purge = news_repo.get_since(NewsType::COMBAT, 0);
  test::expect_true(combat_after_purge.empty());
  std::println(std::cout, "Combat news purged successfully");

  // Purge all
  bool purged_all = news_repo.purge_all();
  test::expect_true(purged_all);

  auto all_announce = news_repo.get_since(NewsType::ANNOUNCE, 0);
  auto all_declaration = news_repo.get_since(NewsType::DECLARATION, 0);
  test::expect_true(all_announce.empty());
  test::expect_true(all_declaration.empty());
  std::println(std::cout, "All news purged successfully");

  // Test post() function (high-level API)
  post(em, "Test message;with special|chars", NewsType::ANNOUNCE);
  auto final_items = news_repo.get_since(NewsType::ANNOUNCE, 0);
  test::expect_eq(final_items.size(), 1);
  // Check that special chars were replaced (';' -> '\n', '|' -> '\t')
  test::expect_contains(final_items[0].message, "\n");
  test::expect_contains(final_items[0].message, "\t");
  std::println(std::cout, "Special character replacement working correctly");

  std::println(std::cout, "\nAll news repository tests passed! ✅");
  return 0;
}
