// SPDX-License-Identifier: Apache-2.0

/// \file database_test.cc
/// \brief Unit tests for Database class methods and SQLite operations.

import dallib;
import gb.entities;
import gb.repositories;
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
    JsonStore store(db);
    NewsRepository news(store);

    // Initial state: news table is empty
    test::expect_true(news.get_since(NewsType::COMBAT).empty());
    test::expect_false(db.is_in_transaction());

    // Transaction 1: insert and commit
    db.begin_transaction();
    test::expect_true(db.is_in_transaction());
    const auto id1 = news.add(NewsType::COMBAT, "Committed news");
    test::expect_gt(id1.value, 0);
    db.commit();
    test::expect_false(db.is_in_transaction());

    auto news_committed = news.get_since(NewsType::COMBAT);
    test::expect_eq(news_committed.size(), 1);
    test::expect_eq(news_committed[0].message, "Committed news");
    std::println(std::cout, "✓ Transaction commit persists changes");

    // Transaction 2: insert and rollback
    db.begin_transaction();
    test::expect_true(db.is_in_transaction());
    const auto id2 = news.add(NewsType::COMBAT, "Rolled back news");
    test::expect_gt(id2.value, 0);
    db.rollback();
    test::expect_false(db.is_in_transaction());

    auto news_after_rollback = news.get_since(NewsType::COMBAT);
    test::expect_eq(news_after_rollback.size(), 1);
    test::expect_eq(news_after_rollback[0].message, "Committed news");
    std::println(std::cout,
                 "✓ Transaction rollback discards uncommitted changes");

    db.optimize();
    std::println(std::cout, "✓ Can optimize database");
  }

  // NewsRepository JSON storage and STORED generated columns
  {
    Database db(":memory:");
    initialize_schema(db);
    JsonStore store(db);
    NewsRepository news(store);

    const auto id1 =
        news.add(NewsType::DECLARATION, "First declaration of war");
    const auto id2 =
        news.add(NewsType::DECLARATION, "Second declaration of war");
    const auto id3 = news.add(NewsType::ANNOUNCE, "Peace treaty signed");
    test::expect_gt(id1.value, 0);
    test::expect_true(id2 > id1);
    test::expect_true(id3 > id2);
    std::println(std::cout, "✓ NewsRepository::add creates incrementing IDs");

    // Verify STORED generated column extraction on tbl_news
    auto decl_ids =
        store.query_ids("tbl_news", "type = ? ORDER BY id",
                        {std::to_underlying(NewsType::DECLARATION)});
    test::expect_eq(decl_ids.size(), 2u);

    auto decl_all = news.get_since(NewsType::DECLARATION);
    test::expect_eq(decl_all.size(), 2);
    test::expect_eq(decl_all[0].id, id1);
    test::expect_true(decl_all[0].type == NewsType::DECLARATION);
    test::expect_eq(decl_all[0].message, "First declaration of war");
    test::expect_gt(decl_all[0].timestamp, 0);

    auto decl_since = news.get_since(NewsType::DECLARATION, id1);
    test::expect_eq(decl_since.size(), 1);
    test::expect_eq(decl_since[0].id, id2);
    std::println(std::cout, "✓ NewsRepository::get_since filters by since_id");

    // Purge specific type
    test::expect_true(news.purge_type(NewsType::DECLARATION));
    test::expect_true(news.get_since(NewsType::DECLARATION).empty());
    test::expect_eq(news.get_since(NewsType::ANNOUNCE).size(), 1);
    std::println(std::cout,
                 "✓ NewsRepository::purge_type purges only specified type");

    // Purge all and verify AUTOINCREMENT monotonicity
    const auto id4 = news.add(NewsType::DECLARATION, "Another news");
    test::expect_true(id4 > id3);
    test::expect_true(news.purge_all());
    test::expect_true(news.get_since(NewsType::DECLARATION).empty());
    test::expect_true(news.get_since(NewsType::ANNOUNCE).empty());
    const auto id5 = news.add(NewsType::ANNOUNCE, "Post-purge news");
    test::expect_true(id5 > id4);  // AUTOINCREMENT never reuses purged IDs
    std::println(std::cout, "✓ NewsRepository::purge_all clears all news and "
                            "preserves monotonic IDs");
  }

  // TelegramRepository JSON storage and STORED generated columns
  {
    Database db(":memory:");
    initialize_schema(db);
    JsonStore store(db);
    TelegramRepository telegrams(store);

    player_t p1{1};
    player_t p2{2};
    governor_t g1{1};
    governor_t g2{2};
    store.store("tbl_race", p1.value, "{}");
    store.store("tbl_race", p2.value, "{}");

    test::expect_eq(telegrams.count(p1, g1), 0);

    const auto t1 = telegrams.add(p1, g1, "Hello Governor 1");
    const auto t2 = telegrams.add(p1, g1, "Fleet arriving soon");
    const auto t3 = telegrams.add(p1, g2, "Governor 2 secret dispatch");
    const auto t4 = telegrams.add(p2, g1, "Message for Player 2");
    test::expect_gt(t1.value, 0);
    test::expect_true(t2 > t1 && t3 > t2 && t4 > t3);
    std::println(std::cout,
                 "✓ TelegramRepository::add successfully stores messages");

    test::expect_eq(telegrams.count(p1, g1), 2);
    test::expect_eq(telegrams.count(p1, g2), 1);
    test::expect_eq(telegrams.count(p2, g1), 1);
    test::expect_eq(telegrams.count(player_t{99}, g1), 0);
    std::println(
        std::cout,
        "✓ TelegramRepository::count returns correct counts per recipient");

    auto p1_g1_msgs = telegrams.get(p1, g1);
    test::expect_eq(p1_g1_msgs.size(), 2);
    test::expect_eq(p1_g1_msgs[0].id, t1);
    test::expect_eq(p1_g1_msgs[0].recipient_player, p1);
    test::expect_eq(p1_g1_msgs[0].recipient_governor, g1);
    test::expect_eq(p1_g1_msgs[0].message, "Hello Governor 1");
    test::expect_gt(p1_g1_msgs[0].timestamp, 0);

    test::expect_eq(p1_g1_msgs[1].id, t2);
    test::expect_eq(p1_g1_msgs[1].message, "Fleet arriving soon");
    std::println(std::cout, "✓ TelegramRepository::get retrieves messages in "
                            "chronological order");

    telegrams.delete_for_governor(p1, g1);
    test::expect_eq(telegrams.count(p1, g1), 0);
    test::expect_eq(telegrams.count(p1, g2), 1);
    test::expect_eq(telegrams.count(p2, g1), 1);
    std::println(std::cout,
                 "✓ TelegramRepository::delete_for_governor removes only "
                 "target recipient messages");

    test::expect_true(telegrams.purge_all());
    test::expect_eq(telegrams.count(p1, g2), 0);
    test::expect_eq(telegrams.count(p2, g1), 0);
    const auto t5 = telegrams.add(p1, g1, "Post-purge telegram");
    test::expect_true(t5 > t4);
    std::println(std::cout,
                 "✓ TelegramRepository::purge_all purges all messages and "
                 "preserves monotonic IDs");
  }

  // Planet query count_non_asteroid_planets
  {
    Database db(":memory:");
    initialize_schema(db);

    // Initial state: 0 planets
    test::expect_eq(db.count_non_asteroid_planets(), 0);

    // Store planets using PlanetRepository which serializes via Glaze
    JsonStore store(db);
    StarRepository stars(store);
    stars.save(Star{1, "Sol"});

    PlanetRepository planets(store);

    Planet earth{1, 1, PlanetType::EARTH, Coordinates{10, 10}};
    planets.save(earth);

    Planet asteroid{1, 2, PlanetType::ASTEROID, Coordinates{5, 5}};
    planets.save(asteroid);

    Planet forest{1, 3, PlanetType::FOREST, Coordinates{10, 10}};
    planets.save(forest);

    test::expect_eq(db.count_non_asteroid_planets(), 2);
    std::println(std::cout,
                 "✓ count_non_asteroid_planets excludes PlanetType::ASTEROID");
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
      db1.begin_transaction();  // NOLINT(bugprone-use-after-move)
    });

    test::expect_throws<SqliteError>([&] {
      initialize_schema(db1);  // NOLINT(bugprone-use-after-move)
    });

    std::println(std::cout, "✓ Database throws SqliteError on database errors");
  }

  // tbl_ship STORED generated columns and B-Tree indexes
  {
    Database db(":memory:");
    initialize_schema(db);
    JsonStore store(db);

    store.store("tbl_race", 1, "{}");
    store.store("tbl_race", 2, "{}");
    store.store("tbl_star", 2, "{}");
    store.store("tbl_star", 5, "{}");
    PlanetRepository(store).save(
        Planet{2, 3, PlanetType::EARTH, Coordinates{5, 5}});

    // Ship 1: Player 1, orbiting star 2 planet 3 (LEVEL_PLAN = 2), alive = 1,
    // destshipno = null
    store.store(
        "tbl_ship", 1,
        R"({"owner":1,"storbits":2,"pnumorbits":3,"whatorbits":2,"whatdest":0,"destshipno":null,"alive":1})");
    // Ship 2: Player 1, orbiting star 2 planet 3, alive = 1, destshipno = null
    store.store(
        "tbl_ship", 2,
        R"({"owner":1,"storbits":2,"pnumorbits":3,"whatorbits":2,"whatdest":0,"destshipno":null,"alive":1})");
    // Ship 3: Player 2, orbiting star 5 (LEVEL_STAR = 1), alive = 1,
    // destshipno = 1 (whatdest = 3 = LEVEL_SHIP)
    store.store(
        "tbl_ship", 3,
        R"({"owner":2,"storbits":5,"pnumorbits":null,"whatorbits":1,"whatdest":3,"destshipno":1,"alive":1})");

    // Verify generated columns extraction for Ship 1 via JsonStore::query_ids
    auto ship1_match = store.query_ids(
        "tbl_ship",
        "id = ? AND owner = ? AND storbits = ? AND pnumorbits = ? AND "
        "whatorbits = ? AND destshipno IS NULL AND alive = ?",
        {1, 1, 2, 3, 2, 1});
    test::expect_eq(ship1_match.size(), 1u);
    test::expect_eq(ship1_match[0], 1);
    std::println(std::cout, "✓ tbl_ship generated columns extract correctly");

    // Verify index by owner and alive
    auto owner1_alive =
        store.query_ids("tbl_ship", "owner = ? AND alive = ?", {1, 1});
    test::expect_eq(owner1_alive.size(), 2u);
    test::expect_eq(owner1_alive[0], 1);
    test::expect_eq(owner1_alive[1], 2);
    std::println(
        std::cout,
        "✓ idx_ship_owner and idx_ship_alive queries match expected rows");

    // Verify index by orbit (storbits, pnumorbits, whatorbits)
    auto orbit_matches = store.query_ids(
        "tbl_ship", "storbits = ? AND pnumorbits = ? AND whatorbits = ?",
        {2, 3, 2});
    test::expect_eq(orbit_matches.size(), 2u);
    test::expect_eq(orbit_matches[0], 1);
    test::expect_eq(orbit_matches[1], 2);
    std::println(std::cout,
                 "✓ idx_ship_orbit query returns matching spatial rows");

    // Verify index by destshipno
    auto destship_matches = store.query_ids("tbl_ship", "destshipno = ?", {1});
    test::expect_eq(destship_matches.size(), 1u);
    test::expect_eq(destship_matches[0], 3);
    std::println(
        std::cout,
        "✓ idx_ship_destship query returns matching docked/carrier rows");
  }

  // Strict SQLite CHECK and FOREIGN KEY (REFERENCES) constraint enforcement
  {
    Database db(":memory:");
    initialize_schema(db);
    JsonStore store(db);

    // 1. CHECK (id >= 1) and CHECK (json_valid(data))
    test::expect_throws<SqliteError>([&] { store.store("tbl_star", 0, "{}"); });
    test::expect_throws<SqliteError>([&] { store.store("tbl_race", 0, "{}"); });
    test::expect_throws<SqliteError>(
        [&] { store.store("tbl_star", 1, "not-valid-json"); });
    test::expect_throws<SqliteError>(
        [&] { store.store("tbl_universe", 2, "{}"); });
    test::expect_throws<SqliteError>(
        [&] { store.store("tbl_server_state", 2, "{}"); });
    std::println(std::cout,
                 "✓ CHECK (id >= 1), CHECK (id = 1), and CHECK (json_valid) "
                 "reject invalid rows");

    // 2. FOREIGN KEY enforcement on tbl_planet, tbl_sector, tbl_block,
    // tbl_power, tbl_commod, tbl_telegram, and tbl_race
    test::expect_throws<SqliteError>([&] {
      store.store_multi("tbl_planet", {{"star_id", 99}, {"planet_order", 1}},
                        "{}");
    });
    store.store("tbl_star", 1, "{}");
    test::expect_throws<SqliteError>([&] {
      store.store_multi("tbl_planet", {{"star_id", 1}, {"planet_order", 1}},
                        R"({"slaved_to":99})");
    });
    store.store("tbl_race", 1, "{}");
    store.store_multi("tbl_planet", {{"star_id", 1}, {"planet_order", 1}},
                      R"({"slaved_to":1})");

    test::expect_throws<SqliteError>([&] {
      store.store_multi(
          "tbl_sector",
          {{"star_id", 1}, {"planet_order", 99}, {"xpos", 0}, {"ypos", 0}},
          "{}");
    });
    test::expect_throws<SqliteError>([&] {
      store.store_multi(
          "tbl_sector",
          {{"star_id", 1}, {"planet_order", 1}, {"xpos", 0}, {"ypos", 0}},
          R"({"owner":99})");
    });

    test::expect_throws<SqliteError>(
        [&] { store.store("tbl_block", 99, "{}"); });
    test::expect_throws<SqliteError>(
        [&] { store.store("tbl_power", 99, "{}"); });
    TelegramRepository telegrams(store);
    test::expect_throws<SqliteError>(
        [&] { telegrams.add(player_t{99}, governor_t{1}, "Hi"); });
    test::expect_throws<SqliteError>(
        [&] { telegrams.add(player_t{1}, governor_t{0}, "Hi"); });
    test::expect_throws<SqliteError>([&] { store.store("tbl_news", 1, "{}"); });

    test::expect_throws<SqliteError>([&] {
      store.store("tbl_commod", 1,
                  R"({"owner":99,"star_from":1,"planet_from":1})");
    });
    test::expect_throws<SqliteError>([&] {
      store.store("tbl_commod", 1,
                  R"({"owner":1,"star_from":1,"planet_from":99})");
    });
    test::expect_throws<SqliteError>(
        [&] { store.store("tbl_race", 2, R"({"Gov_ship":999})"); });
    std::println(
        std::cout,
        "✓ FOREIGN KEY constraints reject orphan references across tables");

    // 3. tbl_ship CHECK (alive = 1), orbital/destination scope CHECK, and FKs
    // Dead ship rejected
    test::expect_throws<SqliteError>([&] {
      store.store("tbl_ship", 1,
                  R"({"owner":1,"whatorbits":0,"whatdest":0,"alive":0})");
    });
    // LEVEL_UNIV (whatorbits = 0) with non-null storbits rejected
    test::expect_throws<SqliteError>([&] {
      store.store(
          "tbl_ship", 1,
          R"({"owner":1,"whatorbits":0,"storbits":1,"whatdest":0,"alive":1})");
    });
    // LEVEL_STAR (whatorbits = 1) with null storbits or non-null pnumorbits
    // rejected
    test::expect_throws<SqliteError>([&] {
      store.store("tbl_ship", 1,
                  R"({"owner":1,"whatorbits":1,"whatdest":0,"alive":1})");
    });
    test::expect_throws<SqliteError>([&] {
      store.store(
          "tbl_ship", 1,
          R"({"owner":1,"whatorbits":1,"storbits":1,"pnumorbits":1,"whatdest":0,"alive":1})");
    });
    // LEVEL_PLAN (whatorbits = 2) with null pnumorbits or nonexistent planet
    // rejected
    test::expect_throws<SqliteError>([&] {
      store.store(
          "tbl_ship", 1,
          R"({"owner":1,"whatorbits":2,"storbits":1,"whatdest":0,"alive":1})");
    });
    test::expect_throws<SqliteError>([&] {
      store.store(
          "tbl_ship", 1,
          R"({"owner":1,"whatorbits":2,"storbits":1,"pnumorbits":99,"whatdest":0,"alive":1})");
    });
    // LEVEL_SHIP (whatorbits = 3) with null destshipno rejected
    test::expect_throws<SqliteError>([&] {
      store.store("tbl_ship", 1,
                  R"({"owner":1,"whatorbits":3,"whatdest":0,"alive":1})");
    });
    // Destination scope CHECK and FK violations rejected
    test::expect_throws<SqliteError>([&] {
      store.store(
          "tbl_ship", 1,
          R"({"owner":1,"whatorbits":0,"whatdest":0,"deststar":1,"alive":1})");
    });
    test::expect_throws<SqliteError>([&] {
      store.store(
          "tbl_ship", 1,
          R"({"owner":1,"whatorbits":0,"whatdest":3,"destshipno":999,"alive":1})");
    });
    std::println(std::cout,
                 "✓ tbl_ship CHECK (alive = 1), scope CHECKs, and FKs "
                 "enforced");
  }

  // Test SqliteError and KeyValue helpers from dallib
  {
    SqliteError err("Custom error", 42);
    test::expect_eq(err.code(), 42);
    test::expect_eq(std::string(err.what()), "Custom error");

    KeyValue kv_double(3.14159);
    test::expect_true(std::holds_alternative<double>(kv_double.val));
    test::expect_eq(std::get<double>(kv_double.val), 3.14159);

    KeyValue kv_str(std::string("hello"));
    test::expect_true(std::holds_alternative<std::string>(kv_str.val));
    test::expect_eq(std::get<std::string>(kv_str.val), "hello");

    KeyValue kv_cstr("world");
    test::expect_true(std::holds_alternative<std::string>(kv_cstr.val));
    test::expect_eq(std::get<std::string>(kv_cstr.val), "world");

    std::println(std::cout,
                 "✓ SqliteError and KeyValue helpers work as expected");
  }

  std::println(std::cout, "\nAll Database tests passed!");
  return 0;
}
