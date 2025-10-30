// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

module;

// Include Glaze first to avoid mixing libc++ header modules with textual
// includes
#include <glaze/glaze.hpp>

import std.compat;

#include <fcntl.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>

#include "gb/files.h"

module gblib;

// Glaze reflection for Commod so we can serialize to JSON
namespace glz {
template <>
struct meta<Commod> {
  using T = Commod;
  static constexpr auto value = object(
      "owner", &T::owner, "governor", &T::governor, "type", &T::type, "amount",
      &T::amount, "deliver", &T::deliver, "bid", &T::bid, "bidder", &T::bidder,
      "bidder_gov", &T::bidder_gov, "star_from", &T::star_from, "planet_from",
      &T::planet_from, "star_to", &T::star_to, "planet_to", &T::planet_to);
};

// Glaze reflection for stardata so we can serialize to JSON
template <>
struct meta<stardata> {
  using T = stardata;
  static constexpr auto value =
      object("numstars", &T::numstars, "ships", &T::ships, "AP", &T::AP,
             "VN_hitlist", &T::VN_hitlist, "VN_index1", &T::VN_index1,
             "VN_index2", &T::VN_index2, "dummy", &T::dummy);
};

// Glaze reflection for block so we can serialize to JSON
template <>
struct meta<block> {
  using T = block;
  static constexpr auto value =
      object("Playernum", &T::Playernum, "name", &T::name, "motto", &T::motto,
             "invite", &T::invite, "pledge", &T::pledge, "atwar", &T::atwar,
             "allied", &T::allied, "next", &T::next, "systems_owned",
             &T::systems_owned, "VPs", &T::VPs, "money", &T::money);
};

// Glaze reflection for power so we can serialize to JSON
template <>
struct meta<power> {
  using T = power;
  static constexpr auto value =
      object("troops", &T::troops, "popn", &T::popn, "resource", &T::resource,
             "fuel", &T::fuel, "destruct", &T::destruct, "ships_owned",
             &T::ships_owned, "planets_owned", &T::planets_owned,
             "sectors_owned", &T::sectors_owned, "money", &T::money, "sum_mob",
             &T::sum_mob, "sum_eff", &T::sum_eff);
};

// Glaze reflection for star_struct so we can serialize to JSON
template <>
struct meta<star_struct> {
  using T = star_struct;
  static constexpr auto value =
      object("ships", &T::ships, "name", &T::name, "governor", &T::governor,
             "AP", &T::AP, "explored", &T::explored, "inhabited", &T::inhabited,
             "xpos", &T::xpos, "ypos", &T::ypos, "numplanets", &T::numplanets,
             "pnames", &T::pnames, "stability", &T::stability, "nova_stage",
             &T::nova_stage, "temperature", &T::temperature, "gravity",
             &T::gravity, "star_id", &T::star_id, "dummy", &T::dummy);
};

// Glaze reflection for Sector so we can serialize to JSON
template <>
struct meta<Sector> {
  using T = Sector;
  static constexpr auto value = object(
      "x", &T::x, "y", &T::y, "eff", &T::eff, "fert", &T::fert,
      "mobilization", &T::mobilization, "crystals", &T::crystals, "resource",
      &T::resource, "popn", &T::popn, "troops", &T::troops, "owner", &T::owner,
      "race", &T::race, "type", &T::type, "condition", &T::condition);
};

// Note: Glaze reflections for plroute, plinfo, and Planet have been moved to
// gb/repositories/gblib-repositories.cppm as part of PlanetRepository
}  // namespace glz

static int commoddata, racedata, shdata, stdata;

static void start_bulk_insert();
static void end_bulk_insert();

void close_file(int fd) { close(fd); }

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

void Sql::getsdata(stardata* S) { ::getsdata(S); }
void getsdata(stardata* S) {
  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT data FROM tbl_stardata WHERE id = 1";

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

      auto stardata_opt = stardata_from_json(json_string);
      if (stardata_opt.has_value()) {
        *S = stardata_opt.value();
        return;
      } else {
        std::println(stderr, "Error: Failed to deserialize stardata from JSON");
      }
    } else {
      std::println(stderr, "Error: NULL JSON data retrieved for stardata");
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty stardata if not found or error
  *S = stardata{};
}

Race Sql::getrace(player_t rnum) { return ::getrace(rnum); };
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
      std::println(stderr, "Error: NULL JSON data retrieved for player {}",
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

Star Sql::getstar(const starnum_t star) { return ::getstar(star); }
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
        std::println(stderr,
                     "Error: Failed to deserialize Star from JSON for star {}",
                     star);
      }
    } else {
      std::println(stderr, "Error: NULL JSON data retrieved for star {}", star);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty star if not found
  return Star(star_struct{});
}

Planet Sql::getplanet(const starnum_t star, const planetnum_t pnum) {
  return ::getplanet(star, pnum);
}
Planet getplanet(const starnum_t star, const planetnum_t pnum) {
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql =
      "SELECT data FROM tbl_planet WHERE star_id=?1 AND planet_order=?2";
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  sqlite3_bind_int(stmt, 1, star);
  sqlite3_bind_int(stmt, 2, pnum);

  auto result = sqlite3_step(stmt);
  if (result != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    throw std::runtime_error("Database unable to return the requested planet");
  }

  // Deserialize Planet from JSON
  const char* json_data =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
  auto planet_result = planet_from_json(json_data);
  sqlite3_finalize(stmt);

  if (!planet_result.has_value()) {
    throw std::runtime_error("Failed to deserialize planet from JSON");
  }

  return std::move(planet_result.value());
}

Sector getsector(const Planet& p, const int x, const int y) {
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql =
      "SELECT data FROM tbl_sector "
      "WHERE planet_id=?1 AND xpos=?2 AND ypos=?3";
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  sqlite3_bind_int(stmt, 1, p.planet_id);
  sqlite3_bind_int(stmt, 2, x);
  sqlite3_bind_int(stmt, 3, y);

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
        std::println(
            stderr, "Error: Failed to deserialize Sector from JSON for planet {} at ({}, {})",
            p.planet_id, x, y);
      }
    } else {
      std::println(stderr, "Error: NULL JSON data retrieved for sector at planet {} ({}, {})",
                   p.planet_id, x, y);
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
  const char* tail = nullptr;
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "SELECT data FROM tbl_sector "
      "WHERE planet_id=?1 ORDER BY ypos, xpos";

  if (dbconn == nullptr) {
    std::println(stderr, "FATAL: getsmap called with NULL database connection");
    exit(-1);
  }
  
  int rc = sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  if (rc != SQLITE_OK) {
    std::println(stderr, "FATAL: sqlite3_prepare_v2 failed in getsmap: {}",
                 sqlite3_errmsg(dbconn));
    exit(-1);
  }

  sqlite3_bind_int(stmt, 1, p.planet_id);

  SectorMap smap(p);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* json_data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    
    if (json_data != nullptr) {
      std::string json_string(json_data);
      auto sector_opt = sector_from_json(json_string);
      if (sector_opt.has_value()) {
        smap.put(std::move(sector_opt.value()));
      } else {
        std::println(stderr, 
                     "FATAL: Failed to deserialize Sector from JSON for planet {}", 
                     p.planet_id);
        exit(-1);
      }
    }
  }

  sqlite3_clear_bindings(stmt);
  sqlite3_reset(stmt);
  sqlite3_finalize(stmt);

  return smap;
}

std::optional<Ship> getship(std::string_view shipstring) {
  auto shipnum = string_to_shipnum(shipstring);
  if (!shipnum) return {};
  return ::getship(*shipnum);
}
std::optional<Ship> Sql::getship(const shipnum_t shipnum) {
  return ::getship(shipnum);
}
std::optional<Ship> getship(const shipnum_t shipnum) {
  return getship(nullptr, shipnum);
}

std::optional<Ship> Sql::getship(Ship** s, const shipnum_t shipnum) {
  return ::getship(s, shipnum);
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

      // Deserialize from JSON
      Ship ship{};
      auto result = glz::read_json(ship, json_string);
      if (!result) {
        // Handle the optional Ship** parameter for compatibility
        if (s != nullptr) {
          if ((*s = (Ship*)malloc(sizeof(Ship))) == nullptr) {
            printf("getship:malloc() error \n");
            exit(0);
          }
          **s = ship;
        }
        
        return ship;
      } else {
        std::println(
            stderr,
            "Error: Failed to deserialize Ship from JSON for ship {}",
            shipnum);
      }
    } else {
      std::println(stderr, "Error: NULL JSON data retrieved for ship {}",
                   shipnum);
      sqlite3_finalize(stmt);
    }
  } else {
    sqlite3_finalize(stmt);
  }

  // Return empty optional if not found
  return {};
}

Commod Sql::getcommod(commodnum_t commodnum) { return ::getcommod(commodnum); }
Commod getcommod(commodnum_t commodnum) {
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
      std::println(stderr, "Error: NULL JSON data retrieved for commod {}",
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

void Sql::putsdata(stardata* S) { ::putsdata(S); }
void putsdata(stardata* S) {
  // Serialize stardata to JSON using existing function
  auto json_result = stardata_to_json(*S);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize stardata to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "REPLACE INTO tbl_stardata (id, data) VALUES (1, ?1)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_text(stmt, 1, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(stderr, "SQLite error in putsdata: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void Sql::putrace(const Race& r) { ::putrace(r); }
void putrace(const Race& r) {
  // Serialize Race to JSON using existing function
  auto json_result = race_to_json(r);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Race to JSON");
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
    std::println(stderr, "SQLite error in putrace: {}", sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void Sql::putstar(const Star& star, starnum_t snum) { ::putstar(star, snum); }
void putstar(const Star& star, starnum_t snum) {
  star_struct s = star.get_struct();

  // Serialize Star to JSON
  auto json_result = star_to_json(s);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Star {} to JSON", snum);
    return;
  }

  // Write to SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "REPLACE INTO tbl_star (id, data) VALUES (?1, ?2)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, snum);
  sqlite3_bind_text(stmt, 2, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(stderr, "SQLite error in putstar: {}", sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

static void start_bulk_insert() {
  char* err_msg = nullptr;
  sqlite3_exec(dbconn, "BEGIN TRANSACTION", nullptr, nullptr, &err_msg);
}

static void end_bulk_insert() {
  char* err_msg = nullptr;
  sqlite3_exec(dbconn, "END TRANSACTION", nullptr, nullptr, &err_msg);
}

void Sql::putplanet(const Planet& p, const Star& star, const planetnum_t pnum) {
  ::putplanet(p, star, pnum);
}
void putplanet(const Planet& p, const Star& s, const planetnum_t pnum) {
  // Serialize Planet to JSON
  auto json_result = planet_to_json(p);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Planet to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail = nullptr;
  sqlite3_stmt* stmt;
  const char* sql =
      "REPLACE INTO tbl_planet (id, star_id, planet_order, data) "
      "VALUES (?1, ?2, ?3, ?4)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);

  auto star = s.get_struct();
  sqlite3_bind_int(stmt, 1, p.planet_id);
  sqlite3_bind_int(stmt, 2, star.star_id);
  sqlite3_bind_int(stmt, 3, pnum);
  sqlite3_bind_text(stmt, 4, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(stderr, "SQLite error in putplanet: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void putsector(const Sector& s, const Planet& p) { putsector(s, p, s.x, s.y); }

void putsector(const Sector& s, const Planet& p, const int x, const int y) {
  // Serialize Sector to JSON using existing function
  auto json_result = sector_to_json(s);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Sector to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail = nullptr;
  sqlite3_stmt* stmt;
  const char* sql =
      "REPLACE INTO tbl_sector (planet_id, xpos, ypos, data) "
      "VALUES (?1, ?2, ?3, ?4)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, p.planet_id);
  sqlite3_bind_int(stmt, 2, x);
  sqlite3_bind_int(stmt, 3, y);
  sqlite3_bind_text(stmt, 4, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(stderr, "SQLite error in putsector: {}", sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void putsmap(const SectorMap& map, const Planet& p) {
  start_bulk_insert();

  for (int y = 0; y < p.Maxy; y++) {
    for (int x = 0; x < p.Maxx; x++) {
      auto& sec = map.get(x, y);
      putsector(sec, p, x, y);
    }
  }

  end_bulk_insert();
}

void Sql::putship(Ship* s) { ::putship(*s); }
void putship(const Ship& s) {
  // Serialize Ship to JSON
  auto json_result = glz::write_json(s);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Ship to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "REPLACE INTO tbl_ship (id, data) VALUES (?1, ?2)";

  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, s.number);
  sqlite3_bind_text(stmt, 2, json_result.value().c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::println(stderr, "SQLite error in putship: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

void Sql::putcommod(const Commod& c, int commodnum) {
  return ::putcommod(c, commodnum);
}
void putcommod(const Commod& c, int commodnum) {
  // Serialize Commod to JSON using existing function
  auto json_result = commod_to_json(c);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Commod to JSON");
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
    std::println(stderr, "SQLite error in putcommod: {}",
                 sqlite3_errmsg(dbconn));
  }

  sqlite3_finalize(stmt);
}

player_t Sql::Numraces() {
  const char* tail = nullptr;
  sqlite3_stmt* stmt;

  const auto sql = "SELECT COUNT(*) FROM tbl_race;";

  int err = sqlite3_prepare_v2(dbconn, sql, -1, &stmt, &tail);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite error in Numraces prepare: {}",
                 sqlite3_errmsg(dbconn));
    return 0;
  }

  player_t count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count;
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

int Sql::Numcommods() {
  struct stat buffer;

  fstat(commoddata, &buffer);
  return ((int)(buffer.st_size / sizeof(Commod)));
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
void clr_shipfree() { fclose(fopen(SHIPFREEDATAFL, "w+")); }

void clr_commodfree() { fclose(fopen(COMMODFREEDATAFL, "w+")); }

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
      std::println(stderr, "Error: Failed to serialize power {} to JSON", i);
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
      std::println(stderr, "Error storing power {}: {}", i,
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
      std::println(stderr, "Invalid player_id {} in tbl_power", i);
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
        std::println(stderr, "Error: Failed to deserialize power {} from JSON",
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
      std::println(stderr, "Error: Failed to serialize block {} to JSON", i);
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
      std::println(stderr, "SQLite error in Putblock for player {}: {}", i,
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
      std::println(stderr, "Error: Invalid data for block player {}",
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

// JSON serialization functions for stardata
std::optional<std::string> stardata_to_json(const stardata& sdata) {
  auto result = glz::write_json(sdata);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<stardata> stardata_from_json(const std::string& json_str) {
  stardata sdata{};
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
  auto result = glz::write_json(sector);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Sector> sector_from_json(const std::string& json_str) {
  Sector sector{};
  auto result = glz::read_json(sector, json_str);
  if (!result) {
    return sector;
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
  auto result = glz::write_json(planet);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Planet> planet_from_json(const std::string& json_str) {
  Planet planet{};
  auto result = glz::read_json(planet, json_str);
  if (!result) {
    return planet;
  }
  return std::nullopt;
}
