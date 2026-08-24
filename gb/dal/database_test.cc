// SPDX-License-Identifier: Apache-2.0

/// \file database_test.cc
/// \brief Unit tests for Database class methods and SQLite operations.

import dallib;
import test;
import std;

int main() {
  std::println(std::cout, "Testing Database class...");

  // Create in-memory database
  {
    Database db(":memory:");
    test::expect_true(db.is_open());
    std::println(std::cout, "✓ Can create in-memory database");
  }

  // Create file-based database
  {
    const std::string test_db = "/tmp/test_database.db";

    // Clean up if exists (ignore errors)
    std::remove(test_db.c_str());

    {
      Database db(test_db);
      test::expect_true(db.is_open());
      std::println(std::cout, "✓ Can create file-based database");
    }

    // Verify file was created
    std::FILE* file = std::fopen(test_db.c_str(), "r");
    test::expect_ne(file, nullptr);
    std::fclose(file);
    std::println(std::cout, "✓ Database file was created");

    // Clean up
    std::remove(test_db.c_str());
  }

  // Transaction support with rollback verification
  {
    Database db(":memory:");
    initialize_schema(db);

    // Initial state: news table is empty
    test::expect_eq(db.news_get_latest_id(1), 0);

    // Transaction 1: insert and commit
    db.begin_transaction();
    auto id1 = db.news_add(1, "Committed news", 1000);
    test::expect_true(id1.has_value());
    db.commit();

    auto news_committed = db.news_get_since(1, 0);
    test::expect_eq(news_committed.size(), 1);
    test::expect_eq(std::get<2>(news_committed[0]), "Committed news");
    std::println(std::cout, "✓ Transaction commit persists changes");

    // Transaction 2: insert and rollback
    db.begin_transaction();
    auto id2 = db.news_add(1, "Rolled back news", 2000);
    test::expect_true(id2.has_value());
    db.rollback();

    auto news_after_rollback = db.news_get_since(1, 0);
    test::expect_eq(news_after_rollback.size(), 1);
    test::expect_eq(std::get<2>(news_after_rollback[0]), "Committed news");
    std::println(std::cout,
                 "✓ Transaction rollback discards uncommitted changes");

    db.optimize();
    std::println(std::cout, "✓ Can optimize database");
  }

  // News operations
  {
    Database db(":memory:");
    initialize_schema(db);

    // Test news_add
    auto id1 = db.news_add(1, "First declaration of war", 100);
    auto id2 = db.news_add(1, "Second declaration of war", 200);
    auto id3 = db.news_add(2, "Peace treaty signed", 300);
    test::expect_true(id1.has_value() && *id1 > 0);
    test::expect_true(id2.has_value() && *id2 > *id1);
    test::expect_true(id3.has_value() && *id3 > *id2);
    std::println(std::cout, "✓ news_add creates incrementing IDs");

    // Test news_get_latest_id
    test::expect_eq(db.news_get_latest_id(1), *id2);
    test::expect_eq(db.news_get_latest_id(2), *id3);
    test::expect_eq(db.news_get_latest_id(99), 0);  // Unused type
    std::println(std::cout, "✓ news_get_latest_id returns correct ID per type");

    // Test news_get_since
    auto type1_all = db.news_get_since(1, 0);
    test::expect_eq(type1_all.size(), 2);
    test::expect_eq(std::get<0>(type1_all[0]), *id1);
    test::expect_eq(std::get<1>(type1_all[0]), 1);
    test::expect_eq(std::get<2>(type1_all[0]), "First declaration of war");
    test::expect_eq(std::get<3>(type1_all[0]), 100);

    auto type1_since = db.news_get_since(1, *id1);
    test::expect_eq(type1_since.size(), 1);
    test::expect_eq(std::get<0>(type1_since[0]), *id2);
    std::println(std::cout, "✓ news_get_since filters by since_id");

    // Test news_purge_type
    test::expect_true(db.news_purge_type(1));
    test::expect_true(db.news_get_since(1, 0).empty());
    test::expect_eq(db.news_get_since(2, 0).size(), 1);  // Type 2 remains
    std::println(std::cout, "✓ news_purge_type purges only specified type");

    // Test news_purge_all
    db.news_add(1, "Another news", 400);
    test::expect_true(db.news_purge_all());
    test::expect_true(db.news_get_since(1, 0).empty());
    test::expect_true(db.news_get_since(2, 0).empty());
    std::println(std::cout, "✓ news_purge_all clears all news");
  }

  // Telegram operations
  {
    Database db(":memory:");
    initialize_schema(db);

    player_t p1{1};
    player_t p2{2};
    governor_t g0{0};
    governor_t g1{1};

    // Test telegram_count on empty
    test::expect_eq(db.telegram_count(p1, g0), 0);

    // Test telegram_add
    auto t1 = db.telegram_add(p1, g0, "Hello Governor 0", 1000);
    auto t2 = db.telegram_add(p1, g0, "Fleet arriving soon", 1050);
    auto t3 = db.telegram_add(p1, g1, "Governor 1 secret dispatch", 1100);
    auto t4 = db.telegram_add(p2, g0, "Message for Player 2", 1200);
    test::expect_true(t1.has_value() && t2.has_value() && t3.has_value() &&
                      t4.has_value());
    std::println(std::cout, "✓ telegram_add successfully stores messages");

    // Test telegram_count
    test::expect_eq(db.telegram_count(p1, g0), 2);
    test::expect_eq(db.telegram_count(p1, g1), 1);
    test::expect_eq(db.telegram_count(p2, g0), 1);
    test::expect_eq(db.telegram_count(player_t{99}, g0), 0);
    std::println(std::cout,
                 "✓ telegram_count returns correct counts per recipient");

    // Test telegram_get
    auto p1_g0_msgs = db.telegram_get(p1, g0);
    test::expect_eq(p1_g0_msgs.size(), 2);
    test::expect_eq(std::get<0>(p1_g0_msgs[0]), *t1);
    test::expect_eq(std::get<1>(p1_g0_msgs[0]), p1.value);
    test::expect_eq(std::get<2>(p1_g0_msgs[0]), g0.value);
    test::expect_eq(std::get<3>(p1_g0_msgs[0]), "Hello Governor 0");
    test::expect_eq(std::get<4>(p1_g0_msgs[0]), 1000);

    test::expect_eq(std::get<0>(p1_g0_msgs[1]), *t2);
    test::expect_eq(std::get<3>(p1_g0_msgs[1]), "Fleet arriving soon");
    std::println(std::cout,
                 "✓ telegram_get retrieves messages in chronological order");

    // Test telegram_delete_for_governor
    test::expect_true(db.telegram_delete_for_governor(p1, g0));
    test::expect_eq(db.telegram_count(p1, g0), 0);
    test::expect_eq(db.telegram_count(p1, g1), 1);  // Other governor untouched
    test::expect_eq(db.telegram_count(p2, g0), 1);  // Other player untouched
    std::println(std::cout, "✓ telegram_delete_for_governor removes only "
                            "target recipient messages");

    // Test telegram_purge_all
    test::expect_true(db.telegram_purge_all());
    test::expect_eq(db.telegram_count(p1, g1), 0);
    test::expect_eq(db.telegram_count(p2, g0), 0);
    std::println(std::cout, "✓ telegram_purge_all purges all messages");
  }

  // Planet query count_non_asteroid_planets
  {
    Database db(":memory:");
    initialize_schema(db);

    // Initial state: 0 planets
    test::expect_eq(db.count_non_asteroid_planets(), 0);

    // Store planet JSON using JsonStore
    JsonStore store(db);
    std::vector<std::pair<std::string, KeyValue>> k1 = {{"star_id", 0},
                                                        {"planet_order", 0}};
    std::vector<std::pair<std::string, KeyValue>> k2 = {{"star_id", 0},
                                                        {"planet_order", 1}};
    std::vector<std::pair<std::string, KeyValue>> k3 = {{"star_id", 0},
                                                        {"planet_order", 2}};

    // PlanetType enum serializes as string (e.g. "ASTEROID")
    store.store_multi("tbl_planet", k1,
                      R"({"type": "EARTH", "name": "Earth"})");
    store.store_multi("tbl_planet", k2,
                      R"({"type": "ASTEROID", "name": "Asteroid1"})");
    store.store_multi("tbl_planet", k3,
                      R"({"type": "FOREST", "name": "Forest"})");

    test::expect_eq(db.count_non_asteroid_planets(), 2);
    std::println(std::cout,
                 "✓ count_non_asteroid_planets excludes type 7 asteroids");
  }

  // Move semantics
  {
    Database db1(":memory:");
    test::expect_true(db1.is_open());

    Database db2(std::move(db1));
    test::expect_true(db2.is_open());
    test::expect_false(db1.is_open());  // NOLINT(bugprone-use-after-move)
    std::println(std::cout, "✓ Move constructor works");

    Database db3(":memory:");
    db3 = std::move(db2);
    test::expect_true(db3.is_open());
    test::expect_false(db2.is_open());  // NOLINT(bugprone-use-after-move)
    std::println(std::cout, "✓ Move assignment works");
  }

  // Destructor closes connection
  {
    Database db(":memory:");
    test::expect_true(db.is_open());
  }
  std::println(std::cout, "✓ Destructor closes connection");

  // SqliteError exception throwing
  {
    Database db1(":memory:");
    Database db2(std::move(db1));  // db1 is now closed/moved-from

    test::expect_throws<SqliteError>([&] {
      db1.news_add(1, "Test Message", 100);  // NOLINT(bugprone-use-after-move)
    });

    test::expect_throws<SqliteError>([&] {
      initialize_schema(db1);  // NOLINT(bugprone-use-after-move)
    });

    std::println(std::cout, "✓ Database throws SqliteError on database errors");
  }

  std::println(std::cout, "\nAll Database tests passed!");
  return 0;
}
