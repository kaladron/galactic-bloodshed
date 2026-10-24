// SPDX-License-Identifier: Apache-2.0

/// \file news_test.cc
/// \brief Unit tests for NewsRepository CRUD, category filtering, paging, and
/// special character sanitization.

import dallib;
import gb.entities;
import gb.services;
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
  const auto id1 = news_repo.add(NewsType::ANNOUNCE, "Server started\n");
  test::expect_gt(id1.value, 0);
  std::println(std::cout, "Added announcement with ID: {}", id1);

  const auto id2 = news_repo.add(NewsType::COMBAT, "Battle between races!\n");
  test::expect_true(id2 > id1);
  std::println(std::cout, "Added combat news with ID: {}", id2);

  const auto id3 = news_repo.add(NewsType::DECLARATION, "Alliance formed\n");
  test::expect_true(id3 > id2);
  std::println(std::cout, "Added declaration with ID: {}", id3);

  // Retrieve news items
  auto announce_items = news_repo.get_since(NewsType::ANNOUNCE);
  test::expect_eq(announce_items.size(), 1);
  test::expect_eq(announce_items[0].id, id1);
  test::expect_eq(announce_items[0].message, "Server started\n");
  std::println(std::cout, "Retrieved {} announcement(s)",
               announce_items.size());

  auto combat_items = news_repo.get_since(NewsType::COMBAT);
  test::expect_eq(combat_items.size(), 1);
  test::expect_eq(combat_items[0].id, id2);
  test::expect_eq(combat_items[0].message, "Battle between races!\n");
  std::println(std::cout, "Retrieved {} combat news item(s)",
               combat_items.size());

  // Pagination - add more items and test get_since
  const auto id4 = news_repo.add(NewsType::ANNOUNCE, "Update completed\n");
  test::expect_true(id4 > id3);

  auto new_announce_items = news_repo.get_since(NewsType::ANNOUNCE, id1);
  test::expect_eq(new_announce_items.size(), 1);
  test::expect_eq(new_announce_items[0].id, id4);
  test::expect_eq(new_announce_items[0].message, "Update completed\n");
  std::println(std::cout, "Retrieved {} new announcement(s) since ID {}",
               new_announce_items.size(), id1);

  // Purge specific type
  test::expect_true(news_repo.purge_type(NewsType::COMBAT));
  auto combat_after_purge = news_repo.get_since(NewsType::COMBAT);
  test::expect_true(combat_after_purge.empty());
  std::println(std::cout, "Combat news purged successfully");

  // Purge all
  test::expect_true(news_repo.purge_all());

  auto all_announce = news_repo.get_since(NewsType::ANNOUNCE);
  auto all_declaration = news_repo.get_since(NewsType::DECLARATION);
  test::expect_true(all_announce.empty());
  test::expect_true(all_declaration.empty());
  std::println(std::cout, "All news purged successfully");

  // Test post() function (high-level API)
  post(em, "Test message;with special|chars", NewsType::ANNOUNCE);
  auto final_items = news_repo.get_since(NewsType::ANNOUNCE);
  test::expect_eq(final_items.size(), 1);
  // Check that special chars were replaced (';' -> '\n', '|' -> '\t')
  test::expect_contains(final_items[0].message, "\n");
  test::expect_contains(final_items[0].message, "\t");
  std::println(std::cout, "Special character replacement working correctly");

  std::println(std::cout, "\nAll news repository tests passed! ✅");
  return 0;
}
