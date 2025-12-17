// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

module;

import std.compat;
import glaze.json;

#include <fcntl.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>

#include "gb/files.h"

module gblib;

// Note: All Glaze reflections have been moved to
// gb/repositories/gblib-repositories.cppm as part of the Repository classes:
// - Commod -> CommodRepository
// - universe_struct -> UniverseRepository
// - block -> BlockRepository
// - power -> PowerRepository
// - star_struct -> StarRepository
// - Sector -> SectorRepository
// - plroute, plinfo, Planet -> PlanetRepository
// - Ship and related types -> ShipRepository
// - Race, toggletype, Race::gov -> RaceRepository

static int commoddata, racedata, shdata, stdata;

static void start_bulk_insert();
static void end_bulk_insert();

void close_file(int fd) {
  close(fd);
}

void openstardata(int* fd) {
  /*printf(" openstardata\n");*/
  if ((*fd = open(STARDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openstardata");
    printf("unable to open %s\n", STARDATAFL);
    exit(-1);
  }
}

void openshdata(int* fd) {
  if ((*fd = open(SHIPDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openshdata");
    printf("unable to open %s\n", SHIPDATAFL);
    exit(-1);
  }
}

void opencommoddata(int* fd) {
  if ((*fd = open(COMMODDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("opencommoddata");
    printf("unable to open %s\n", COMMODDATAFL);
    exit(-1);
  }
}

void openracedata(int* fd) {
  if ((*fd = open(RACEDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openrdata");
    printf("unable to open %s\n", RACEDATAFL);
    exit(-1);
  }
}

void getsdata(universe_struct* S) {
  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT data FROM tbl_universe WHERE id = 1";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  int result = sqlite3_step(stmt);
  if (result == SQLITE_ROW) {
    // Data found in SQLite, deserialize from JSON
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    if (json_data != nullptr) {
      // Copy the JSON data before finalizing the statement
      std::string json_string(json_data);
      sqlite3_finalize(stmt);

      auto universe_opt = universe_from_json(json_string);
      if (universe_opt.has_value()) {
        *S = universe_opt.value();
        return;
      } else {
        std::println(std::cerr,
                     "Error: Failed to deserialize universe_struct from JSON");
      }
    } else {
      std::println(std::cerr,
                   "Error: NULL JSON data retrieved for universe_struct");
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty universe_struct if not found or error
  *S = universe_struct{};
}

Race getrace(player_t rnum) {
  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT data FROM tbl_race WHERE id = ?1";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, rnum);

  int result = sqlite3_step(stmt);
  if (result == SQLITE_ROW) {
    // Data found in SQLite, deserialize from JSON
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    if (json_data != nullptr) {
      // Copy the JSON data before finalizing the statement
      std::string json_string(json_data);
      sqlite3_finalize(stmt);

      auto race_opt = race_from_json(json_string);
      if (race_opt.has_value()) {
        return race_opt.value();
      } else {
        std::println(
            stderr, "Error: Failed to deserialize Race from JSON for player {}",
            rnum);
      }
    } else {
      std::println(std::cerr, "Error: NULL JSON data retrieved for player {}",
                   rnum);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty race if not found
  Race r{};
  return r;
}

Star getstar(const starnum_t star) {
  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT data FROM tbl_star WHERE id = ?1";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, star);

  int result = sqlite3_step(stmt);
  if (result == SQLITE_ROW) {
    // Data found in SQLite, deserialize from JSON
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    if (json_data != nullptr) {
      // Copy the JSON data before finalizing the statement
      std::string json_string(json_data);
      sqlite3_finalize(stmt);

      auto star_opt = star_from_json(json_string);
      if (star_opt.has_value()) {
        return Star(star_opt.value());
      } else {
        std::println(std::cerr,
                     "Error: Failed to deserialize Star from JSON for star {}",
                     star);
      }
    } else {
      std::println(std::cerr, "Error: NULL JSON data retrieved for star {}", star);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty star if not found
  return Star(star_struct{});
}

Planet getplanet(const starnum_t star, const planetnum_t pnum) {
  throw std::runtime_error(
      "DEPRECATED: getplanet() is removed. Use EntityManager::peek_planet() "
      "or EntityManager::get_planet() instead.");
}

Sector getsector(const Planet& p, const int x, const int y) {
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql =
      "SELECT data FROM tbl_sector "
      "WHERE star_id=?1 AND planet_order=?2 AND xpos=?3 AND ypos=?4";
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  sqlite3_bind_int(stmt, 1, p.star_id());
  sqlite3_bind_int(stmt, 2, p.planet_order());
  sqlite3_bind_int(stmt, 3, x);
  sqlite3_bind_int(stmt, 4, y);

  auto result = sqlite3_step(stmt);
  if (result == SQLITE_ROW) {
    // Data found in SQLite, deserialize from JSON
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    if (json_data != nullptr) {
      // Copy the JSON data before finalizing the statement
      std::string json_string(json_data);
      sqlite3_finalize(stmt);

      auto sector_opt = sector_from_json(json_string);
      if (sector_opt.has_value()) {
        return std::move(sector_opt.value());
      } else {
        std::println(std::cerr,
                     "Error: Failed to deserialize Sector from JSON for planet "
                     "({},{}) at ({}, {})",
                     p.star_id(), p.planet_order(), x, y);
      }
    } else {
      std::println(std::cerr,
                   "Error: NULL JSON data retrieved for sector at planet "
                   "({},{}) at ({}, {})",
                   p.star_id(), p.planet_order(), x, y);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
    throw std::runtime_error("Database unable to return the requested sector");
  }

  // Return empty sector if deserialization failed
  Sector s{};
  return s;
}

SectorMap getsmap(const Planet& p) {
  throw std::runtime_error(
      "DEPRECATED: getsmap() is removed. Use EntityManager::peek_sectormap() "
      "or EntityManager::get_sectormap() instead.");
}

std::optional<Ship> getship(std::string_view shipstring) {
  auto shipnum = string_to_shipnum(shipstring);
  if (!shipnum) return {};
  return ::getship(*shipnum);
}
std::optional<Ship> getship(const shipnum_t shipnum) {
  return getship(nullptr, shipnum);
}

std::optional<Ship> getship(Ship** s, const shipnum_t shipnum) {
  if (shipnum <= 0) return {};

  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT data FROM tbl_ship WHERE id = ?1";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, shipnum);

  int result = sqlite3_step(stmt);
  if (result == SQLITE_ROW) {
    // Data found in SQLite, deserialize from JSON
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    if (json_data != nullptr) {
      // Copy the JSON data before finalizing the statement
      std::string json_string(json_data);
      sqlite3_finalize(stmt);

      // Deserialize from JSON into ship_struct, then wrap in Ship
      ship_struct ship_data{};
      auto result = glz::read_json(ship_data, json_string);
      if (!result) {
        Ship ship{ship_data};
        // Handle the optional Ship** parameter for compatibility
        if (s != nullptr) {
          if ((*s = (Ship*)malloc(sizeof(Ship))) == nullptr) {
            printf("getship:malloc() error \n");
            exit(0);
          }
          // Use placement new to construct Ship in allocated memory
          new (*s) Ship(ship_data);
        }

        return ship;
      } else {
        std::println(std::cerr,
                     "Error: Failed to deserialize Ship from JSON for ship {}",
                     shipnum);
      }
    } else {
      std::println(std::cerr, "Error: NULL JSON data retrieved for ship {}",
                   shipnum);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty optional if not found
  return {};
}

Commod getcommod(commodnum_t commodnum) {
  throw std::runtime_error(
      "DEPRECATED: getcommod() is removed. Use EntityManager::peek_commod() "
      "for read-only access or EntityManager::get_commod() for read-write "
      "access with auto-save.");

  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT data FROM tbl_commod WHERE id = ?1";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, commodnum);

  int result = sqlite3_step(stmt);
  if (result == SQLITE_ROW) {
    // Data found in SQLite, deserialize from JSON
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    if (json_data != nullptr) {
      // Copy the JSON data before finalizing the statement
      std::string json_string(json_data);
      sqlite3_finalize(stmt);

      auto commod_opt = commod_from_json(json_string);
      if (commod_opt.has_value()) {
        return commod_opt.value();
      } else {
        std::println(
            stderr,
            "Error: Failed to deserialize Commod from JSON for commod {}",
            commodnum);
      }
    } else {
      std::println(std::cerr, "Error: NULL JSON data retrieved for commod {}",
                   commodnum);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty commod if not found
  Commod commod{};
  return commod;
}

/* gets the ship # listed in the top of the file SHIPFREEDATAFL. this
** might have no other uses besides build().
*/
int getdeadship() {
  // Find the first gap in ship IDs, or return 1 if no ships exist
  const char* sql = R"(
    WITH RECURSIVE cnt(x) AS (
      SELECT 1
      UNION ALL
      SELECT x+1 FROM cnt
      LIMIT (SELECT IFNULL(MAX(id), 0) + 1 FROM tbl_ship)
    )
    SELECT x FROM cnt
    WHERE x NOT IN (SELECT id FROM tbl_ship)
    ORDER BY x
    LIMIT 1
  )";

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, nullptr);

  int result = 1;  // Default if no ships exist
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return result;
}

int getdeadcommod() {
  // Find the first gap in commod IDs, or return 1 if no commods exist
  const char* sql = R"(
    WITH RECURSIVE cnt(x) AS (
      SELECT 1
      UNION ALL
      SELECT x+1 FROM cnt
      LIMIT (SELECT IFNULL(MAX(id), 0) + 1 FROM tbl_commod)
    )
    SELECT x FROM cnt
    WHERE x NOT IN (SELECT id FROM tbl_commod)
    ORDER BY x
    LIMIT 1
  )";

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, nullptr);

  int result = 1;  // Default if no commods exist
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return result;
}

void putsdata(universe_struct* S) {
  // Serialize universe_struct to JSON using existing function
  auto json_result = universe_to_json(*S);
  if (!json_result.has_value()) {
    std::println(std::cerr, "Error: Failed to serialize universe_struct to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "REPLACE INTO tbl_universe (id, data) VALUES (1, ?1)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_text(stmt, 1, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(std::cerr, "SQLite error in putsdata: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void putrace(const Race& r) {
  // Serialize Race to JSON using existing function
  auto json_result = race_to_json(r);
  if (!json_result.has_value()) {
    std::println(std::cerr, "Error: Failed to serialize Race to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "REPLACE INTO tbl_race (id, data) VALUES (?1, ?2)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, r.Playernum);
  sqlite3_bind_text(stmt, 2, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(std::cerr, "SQLite error in putrace: {}", sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void putstar(const Star& star, starnum_t snum) {
  throw std::runtime_error(
      "DEPRECATED: putstar() is removed. Use EntityManager::get_star() which "
      "auto-saves when the handle goes out of scope.");
}

static void start_bulk_insert() {
  char* err_msg = nullptr;
  sqlite3_exec(dbconn, "BEGIN TRANSACTION", nullptr, nullptr, &err_msg);
}

static void end_bulk_insert() {
  char* err_msg = nullptr;
  sqlite3_exec(dbconn, "END TRANSACTION", nullptr, nullptr, &err_msg);
}

void putplanet(const Planet& p, const Star& s, const planetnum_t pnum) {
  // Serialize Planet to JSON
  auto json_result = planet_to_json(p);
  if (!json_result.has_value()) {
    std::println(std::cerr, "Error: Failed to serialize Planet to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail = nullptr;
  sqlite3_stmt* stmt;
  const char* sql =
      "INSERT OR REPLACE INTO tbl_planet (star_id, planet_order, data) "
      "VALUES (?1, ?2, ?3)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  auto star = s.get_struct();
  sqlite3_bind_int(stmt, 1, star.star_id);
  sqlite3_bind_int(stmt, 2, pnum);
  sqlite3_bind_text(stmt, 3, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(std::cerr, "SQLite error in putplanet: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void putsector(const Sector& s, const Planet& p) {
  putsector(s, p, s.get_x(), s.get_y());
}

void putsector(const Sector& s, const Planet& p, const int x, const int y) {
  // Serialize Sector to JSON using existing function
  auto json_result = sector_to_json(s);
  if (!json_result.has_value()) {
    std::println(std::cerr, "Error: Failed to serialize Sector to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail = nullptr;
  sqlite3_stmt* stmt;
  const char* sql = "INSERT OR REPLACE INTO tbl_sector (star_id, planet_order, "
                    "xpos, ypos, data) "
                    "VALUES (?1, ?2, ?3, ?4, ?5)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, p.star_id());
  sqlite3_bind_int(stmt, 2, p.planet_order());
  sqlite3_bind_int(stmt, 3, x);
  sqlite3_bind_int(stmt, 4, y);
  sqlite3_bind_text(stmt, 5, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(std::cerr, "SQLite error in putsector: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void putsmap(const SectorMap& map, const Planet& p) {
  throw std::runtime_error(
      "DEPRECATED: putsmap() is removed. Use SectorRepository::save_map() "
      "instead.");
}

void putship(const Ship& s) {
  throw std::runtime_error(
      "DEPRECATED: putship() is removed. Use EntityManager::get_ship() which "
      "auto-saves when the handle goes out of scope.");
}

void putcommod(const Commod& c, int commodnum) {
  throw std::runtime_error(
      "DEPRECATED: putcommod() is removed. Use EntityManager::get_commod() "
      "which auto-saves when the handle goes out of scope.");

  // Serialize Commod to JSON using existing function
  auto json_result = commod_to_json(c);
  if (!json_result.has_value()) {
    std::println(std::cerr, "Error: Failed to serialize Commod to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "REPLACE INTO tbl_commod (id, data) VALUES (?1, ?2)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, commodnum);
  sqlite3_bind_text(stmt, 2, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(std::cerr, "SQLite error in putcommod: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

shipnum_t Numships() /* return number of ships */
{
  const char* tail = nullptr;
  sqlite3_stmt* stmt;

  const auto sql = "SELECT COUNT(*) FROM tbl_ship;";
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  auto result = sqlite3_step(stmt);
  if (result != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    throw std::runtime_error("Database unable to return the number of ships");
  }

  shipnum_t count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return count;
}

off_t getnewslength(NewsType type) {
  struct stat buffer;
  FILE* fp;

  switch (type) {
    using enum NewsType;
    case DECLARATION:
      if ((fp = fopen(DECLARATIONFL, "r")) == nullptr)
        fp = fopen(DECLARATIONFL, "w+");
      break;

    case TRANSFER:
      if ((fp = fopen(TRANSFERFL, "r")) == nullptr)
        fp = fopen(TRANSFERFL, "w+");
      break;
    case COMBAT:
      if ((fp = fopen(COMBATFL, "r")) == nullptr) fp = fopen(COMBATFL, "w+");
      break;
    case ANNOUNCE:
      if ((fp = fopen(ANNOUNCEFL, "r")) == nullptr)
        fp = fopen(ANNOUNCEFL, "w+");
      break;
    default:
      return 0;
  }
  fstat(fileno(fp), &buffer);
  fclose(fp);
  return (buffer.st_size);
}

/* delete contents of dead ship file */
void clr_shipfree() {
  fclose(fopen(SHIPFREEDATAFL, "w+"));
}

void clr_commodfree() {
  fclose(fopen(COMMODFREEDATAFL, "w+"));
}

/*
** writes the ship to the dead ship file at its end.
*/
void makeshipdead(int shipnum) {
  if (shipnum == 0) return;

  const char* sql = "DELETE FROM tbl_ship WHERE id = ?";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, shipnum);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void makecommoddead(int commodnum) {
  if (commodnum == 0) return;

  const char* sql = "DELETE FROM tbl_commod WHERE id = ?";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, commodnum);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void putpower(power p[MAXPLAYERS]) {
  // Store each power struct in SQLite database as JSON
  for (player_t i = 1; i <= MAXPLAYERS; i++) {
    auto json_result = power_to_json(p[i - 1]);
    if (!json_result.has_value()) {
      std::println(std::cerr, "Error: Failed to serialize power {} to JSON", i);
      continue;
    }

    const char* tail;
    sqlite3_stmt* stmt;
    const char* sql = "REPLACE INTO tbl_power (id, data) VALUES (?1, ?2);";
    sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
    sqlite3_bind_int(stmt, 1, i);
    sqlite3_bind_text(stmt, 2, json_result.value().c_str(), -1,
                      SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      std::println(std::cerr, "Error storing power {}: {}", i,
                   sqlite3_errmsg(dbconn));
    }

    sqlite3_finalize(stmt);
  }
}

void getpower(power p[MAXPLAYERS]) {
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, data FROM tbl_power";
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    player_t i = sqlite3_column_int(stmt, 0);
    if (i < 1 || i > MAXPLAYERS) {
      std::println(std::cerr, "Invalid player_id {} in tbl_power", i);
      continue;
    }

    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (json_data) {
      std::string json_string(json_data);
      auto power_opt = power_from_json(json_string);
      if (power_opt.has_value()) {
        p[i - 1] = power_opt.value();
      } else {
        std::println(std::cerr, "Error: Failed to deserialize power {} from JSON",
                     i);
      }
    }
  }

  sqlite3_finalize(stmt);
}

void Putblock(block b[MAXPLAYERS]) {
  // Store each block in SQLite database as JSON
  for (player_t i = 1; i <= MAXPLAYERS; i++) {
    auto json_result = block_to_json(b[i - 1]);
    if (!json_result.has_value()) {
      std::println(std::cerr, "Error: Failed to serialize block {} to JSON", i);
      continue;
    }

    const char* tail;
    sqlite3_stmt* stmt;
    const char* sql = "REPLACE INTO tbl_block (id, data) VALUES (?1, ?2)";

    sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
    sqlite3_bind_int(stmt, 1, i);
    sqlite3_bind_text(stmt, 2, json_result.value().c_str(), -1,
                      SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      std::println(std::cerr, "SQLite error in Putblock for player {}: {}", i,
                   sqlite3_errmsg(dbconn));
    }

    sqlite3_finalize(stmt);
  }
}

void Getblock(block b[MAXPLAYERS]) {
  // Read each block from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, data FROM tbl_block ORDER BY id";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  // Initialize array to empty blocks
  for (player_t i = 0; i < MAXPLAYERS; i++) {
    b[i] = block{};
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    player_t player_id = sqlite3_column_int(stmt, 0);
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

    if (json_data != nullptr && player_id >= 1 && player_id <= MAXPLAYERS) {
      std::string json_string(json_data);
      auto block_opt = block_from_json(json_string);
      if (block_opt.has_value()) {
        b[player_id - 1] = block_opt.value();
      } else {
        std::println(
            stderr,
            "Error: Failed to deserialize block from JSON for player {}",
            player_id);
      }
    } else {
      std::println(std::cerr, "Error: Invalid data for block player {}",
                   player_id);
    }
  }

  sqlite3_finalize(stmt);
}

void open_files() {
  opencommoddata(&commoddata);
  openracedata(&racedata);
  openshdata(&shdata);
  openstardata(&stdata);
}

void close_files() {
  close_file(commoddata);
  close_file(racedata);
  close_file(shdata);
  close_file(stdata);
}

// JSON serialization functions for Race (for SQLite migration)
std::optional<std::string> race_to_json(const Race& race) {
  auto result = glz::write_json(race);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Race> race_from_json(const std::string& json_str) {
  Race race{};
  auto result = glz::read_json(race, json_str);
  if (!result) {
    return race;
  }
  return std::nullopt;
}

// JSON serialization functions for universe_struct
std::optional<std::string> universe_to_json(const universe_struct& sdata) {
  auto result = glz::write_json(sdata);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<universe_struct> universe_from_json(const std::string& json_str) {
  universe_struct sdata{};
  auto result = glz::read_json(sdata, json_str);
  if (!result) {
    return sdata;
  }
  return std::nullopt;
}

// JSON serialization functions for block
std::optional<std::string> block_to_json(const block& b) {
  auto result = glz::write_json(b);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<block> block_from_json(const std::string& json_str) {
  block b{};
  auto result = glz::read_json(b, json_str);
  if (!result) {
    return b;
  }
  return std::nullopt;
}

// JSON serialization functions for power
std::optional<std::string> power_to_json(const power& p) {
  auto result = glz::write_json(p);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<power> power_from_json(const std::string& json_str) {
  power p{};
  auto result = glz::read_json(p, json_str);
  if (!result) {
    return p;
  }
  return std::nullopt;
}

// JSON serialization functions for Commod
std::optional<std::string> commod_to_json(const Commod& commod) {
  auto result = glz::write_json(commod);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Commod> commod_from_json(const std::string& json_str) {
  Commod commod{};
  auto result = glz::read_json(commod, json_str);
  if (!result) {
    return commod;
  }
  return std::nullopt;
}

// JSON serialization functions for Sector
std::optional<std::string> sector_to_json(const Sector& sector) {
  // Serialize the underlying sector_struct
  const sector_struct& data = sector.to_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Sector> sector_from_json(const std::string& json_str) {
  // Deserialize to sector_struct, then wrap in Sector
  sector_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Sector(data);
  }
  return std::nullopt;
}

// JSON serialization functions for star_struct
std::optional<std::string> star_to_json(const star_struct& star) {
  auto result = glz::write_json(star);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<star_struct> star_from_json(const std::string& json_str) {
  star_struct star{};
  auto result = glz::read_json(star, json_str);
  if (!result) {
    return star;
  }
  return std::nullopt;
}

// JSON serialization functions for Planet
std::optional<std::string> planet_to_json(const Planet& planet) {
  // Extract planet_struct from Planet wrapper
  planet_struct data = planet.get_struct();
  auto result = glz::write_json(data);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Planet> planet_from_json(const std::string& json_str) {
  // Deserialize to planet_struct, then wrap in Planet
  planet_struct data{};
  auto result = glz::read_json(data, json_str);
  if (!result) {
    return Planet(data);  // Wrap the planet_struct in Planet
  }
  return std::nullopt;
}
