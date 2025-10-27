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

// Glaze reflection for toggletype struct
template <>
struct meta<toggletype> {
  using T = toggletype;
  static constexpr auto value =
      object("invisible", &T::invisible, "standby", &T::standby, "color",
             &T::color, "gag", &T::gag, "double_digits", &T::double_digits,
             "inverse", &T::inverse, "geography", &T::geography, "autoload",
             &T::autoload, "highlight", &T::highlight, "compat", &T::compat);
};

// Glaze reflection for Race::gov struct
template <>
struct meta<Race::gov> {
  using T = Race::gov;
  static constexpr auto value =
      object("name", &T::name, "password", &T::password, "active", &T::active,
             "deflevel", &T::deflevel, "defsystem", &T::defsystem,
             "defplanetnum", &T::defplanetnum, "homelevel", &T::homelevel,
             "homesystem", &T::homesystem, "homeplanetnum", &T::homeplanetnum,
             "newspos", &T::newspos, "toggle", &T::toggle, "money", &T::money,
             "income", &T::income, "maintain", &T::maintain, "cost_tech",
             &T::cost_tech, "cost_market", &T::cost_market, "profit_market",
             &T::profit_market, "login", &T::login);
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

// Glaze reflection for Race class
template <>
struct meta<Race> {
  using T = Race;
  static constexpr auto value = object(
      "Playernum", &T::Playernum, "name", &T::name, "password", &T::password,
      "info", &T::info, "motto", &T::motto, "absorb", &T::absorb,
      "collective_iq", &T::collective_iq, "pods", &T::pods, "fighters",
      &T::fighters, "IQ", &T::IQ, "IQ_limit", &T::IQ_limit, "number_sexes",
      &T::number_sexes, "fertilize", &T::fertilize, "adventurism",
      &T::adventurism, "birthrate", &T::birthrate, "mass", &T::mass,
      "metabolism", &T::metabolism, "conditions", &T::conditions, "likes",
      &T::likes, "likesbest", &T::likesbest, "dissolved", &T::dissolved, "God",
      &T::God, "Guest", &T::Guest, "Metamorph", &T::Metamorph, "monitor",
      &T::monitor, "translate", &T::translate, "atwar", &T::atwar, "allied",
      &T::allied, "Gov_ship", &T::Gov_ship, "morale", &T::morale, "points",
      &T::points, "controlled_planets", &T::controlled_planets, "victory_turns",
      &T::victory_turns, "turn", &T::turn, "tech", &T::tech, "discoveries",
      &T::discoveries, "victory_score", &T::victory_score, "votes", &T::votes,
      "planet_points", &T::planet_points, "governors", &T::governors,
      "governor", &T::governor);
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

// Glaze reflection for special ship function data structures
template <>
struct meta<AimedAtData> {
  using T = AimedAtData;
  static constexpr auto value = object(
      "shipno", &T::shipno,
      "snum", &T::snum,
      "intensity", &T::intensity,
      "pnum", &T::pnum,
      "level", &T::level);
};

template <>
struct meta<MindData> {
  using T = MindData;
  static constexpr auto value = object(
      "progenitor", &T::progenitor,
      "target", &T::target,
      "generation", &T::generation,
      "busy", &T::busy,
      "tampered", &T::tampered,
      "who_killed", &T::who_killed);
};

template <>
struct meta<PodData> {
  using T = PodData;
  static constexpr auto value = object(
      "decay", &T::decay,
      "temperature", &T::temperature);
};

template <>
struct meta<TimerData> {
  using T = TimerData;
  static constexpr auto value = object("count", &T::count);
};

template <>
struct meta<ImpactData> {
  using T = ImpactData;
  static constexpr auto value = object(
      "x", &T::x,
      "y", &T::y,
      "scatter", &T::scatter);
};

template <>
struct meta<TriggerData> {
  using T = TriggerData;
  static constexpr auto value = object("radius", &T::radius);
};

template <>
struct meta<TerraformData> {
  using T = TerraformData;
  static constexpr auto value = object("index", &T::index);
};

template <>
struct meta<TransportData> {
  using T = TransportData;
  static constexpr auto value = object("target", &T::target);
};

template <>
struct meta<WasteData> {
  using T = WasteData;
  static constexpr auto value = object("toxic", &T::toxic);
};

// Glaze reflection for anonymous structs in Ship class
template <>
struct meta<decltype(Ship::navigate)> {
  using T = decltype(Ship::navigate);
  static constexpr auto value = object(
      "on", &T::on,
      "speed", &T::speed,
      "turns", &T::turns,
      "bearing", &T::bearing);
};

template <>
struct meta<decltype(Ship::protect)> {
  using T = decltype(Ship::protect);
  static constexpr auto value = object(
      "maxrng", &T::maxrng,
      "on", &T::on,
      "planet", &T::planet,
      "self", &T::self,
      "evade", &T::evade,
      "ship", &T::ship);
};

template <>
struct meta<decltype(Ship::hyper_drive)> {
  using T = decltype(Ship::hyper_drive);
  static constexpr auto value = object(
      "charge", &T::charge,
      "ready", &T::ready,
      "on", &T::on,
      "has", &T::has);
};

// Glaze reflection for Ship class  
template <>
struct meta<Ship> {
  using T = Ship;
  static constexpr auto value = object(
      "number", &T::number,
      "owner", &T::owner,
      "governor", &T::governor,
      "name", &T::name,
      "shipclass", &T::shipclass,
      "race", &T::race,
      "xpos", &T::xpos,
      "ypos", &T::ypos,
      "fuel", &T::fuel,
      "mass", &T::mass,
      "land_x", &T::land_x,
      "land_y", &T::land_y,
      "destshipno", &T::destshipno,
      "nextship", &T::nextship,
      "ships", &T::ships,
      "armor", &T::armor,
      "size", &T::size,
      "max_crew", &T::max_crew,
      "max_resource", &T::max_resource,
      "max_destruct", &T::max_destruct,
      "max_fuel", &T::max_fuel,
      "max_speed", &T::max_speed,
      "build_type", &T::build_type,
      "build_cost", &T::build_cost,
      "base_mass", &T::base_mass,
      "tech", &T::tech,
      "complexity", &T::complexity,
      "destruct", &T::destruct,
      "resource", &T::resource,
      "popn", &T::popn,
      "troops", &T::troops,
      "crystals", &T::crystals,
      // Note: 'special' is now a std::variant with native Glaze support
      "special", &T::special,
      "who_killed", &T::who_killed,
      "navigate", &T::navigate,
      "protect", &T::protect,
      "mount", &T::mount,
      "hyper_drive", &T::hyper_drive,
      "cew", &T::cew,
      "cew_range", &T::cew_range,
      "cloak", &T::cloak,
      "laser", &T::laser,
      "focus", &T::focus,
      "fire_laser", &T::fire_laser,
      "storbits", &T::storbits,
      "deststar", &T::deststar,
      "destpnum", &T::destpnum,
      "pnumorbits", &T::pnumorbits,
      "whatdest", &T::whatdest,
      "whatorbits", &T::whatorbits,
      "damage", &T::damage,
      "rad", &T::rad,
      "retaliate", &T::retaliate,
      "target", &T::target,
      "type", &T::type,
      "speed", &T::speed,
      "active", &T::active,
      "alive", &T::alive,
      "mode", &T::mode,
      "bombard", &T::bombard,
      "mounted", &T::mounted,
      "cloaked", &T::cloaked,
      "sheep", &T::sheep,
      "docked", &T::docked,
      "notified", &T::notified,
      "examined", &T::examined,
      "on", &T::on,
      "merchant", &T::merchant,
      "guns", &T::guns,
      "primary", &T::primary,
      "primtype", &T::primtype,
      "secondary", &T::secondary,
      "sectype", &T::sectype,
      "hanger", &T::hanger,
      "max_hanger", &T::max_hanger);
};

// Glaze reflection for plroute struct
template <>
struct meta<plroute> {
  using T = plroute;
  static constexpr auto value =
      object("set", &T::set, "dest_star", &T::dest_star, "dest_planet",
             &T::dest_planet, "load", &T::load, "unload", &T::unload, "x",
             &T::x, "y", &T::y);
};

// Glaze reflection for plinfo so we can serialize to JSON
template <>
struct meta<plinfo> {
  using T = plinfo;
  static constexpr auto value = object(
      "fuel", &T::fuel, "destruct", &T::destruct, "resource", &T::resource,
      "popn", &T::popn, "troops", &T::troops, "crystals", &T::crystals,
      "prod_res", &T::prod_res, "prod_fuel", &T::prod_fuel, "prod_dest",
      &T::prod_dest, "prod_crystals", &T::prod_crystals, "prod_money",
      &T::prod_money, "prod_tech", &T::prod_tech, "tech_invest",
      &T::tech_invest, "numsectsowned", &T::numsectsowned, "comread",
      &T::comread, "mob_set", &T::mob_set, "tox_thresh", &T::tox_thresh,
      "explored", &T::explored, "autorep", &T::autorep, "tax", &T::tax,
      "newtax", &T::newtax, "guns", &T::guns, "route", &T::route, "mob_points",
      &T::mob_points, "est_production", &T::est_production);
};

// Glaze reflection for Planet class so we can serialize to JSON
template <>
struct meta<Planet> {
  using T = Planet;
  static constexpr auto value =
      object("xpos", &T::xpos, "ypos", &T::ypos, "ships", &T::ships, "Maxx",
             &T::Maxx, "Maxy", &T::Maxy, "info", &T::info, "conditions",
             &T::conditions, "popn", &T::popn, "troops", &T::troops, "maxpopn",
             &T::maxpopn, "total_resources", &T::total_resources, "slaved_to",
             &T::slaved_to, "type", &T::type, "expltimer", &T::expltimer,
             "explored", &T::explored, "planet_id", &T::planet_id);
};
}  // namespace glz

static int commoddata, racedata, shdata, stdata;

static void start_bulk_insert();
static void end_bulk_insert();

void close_file(int fd) { close(fd); }

void initsqldata() {  // __attribute__((no_sanitize_memory)) {
  const char* tbl_create = R"(
      CREATE TABLE tbl_planet(
          planet_id INT PRIMARY KEY NOT NULL,
          star_id INT NOT NULL,
          planet_order INT NOT NULL,
          planet_data TEXT NOT NULL);

  CREATE INDEX star_planet ON tbl_planet (star_id, planet_order);

  CREATE TABLE
  tbl_sector(planet_id INT NOT NULL, xpos INT NOT NULL, ypos INT NOT NULL,
             sector_data TEXT, PRIMARY KEY(planet_id, xpos, ypos));

  CREATE TABLE tbl_star(
    star_id INT PRIMARY KEY NOT NULL,
    star_data TEXT NOT NULL);

  CREATE TABLE tbl_power(
      player_id INT PRIMARY KEY NOT NULL,
      power_data TEXT NOT NULL);

  CREATE TABLE tbl_race(
    player_id INT PRIMARY KEY NOT NULL,
    race_data TEXT NOT NULL);

  CREATE TABLE tbl_stardata(
    id INT PRIMARY KEY NOT NULL DEFAULT 1,
    stardata_json TEXT NOT NULL);

  CREATE TABLE tbl_block(
    player_id INT PRIMARY KEY NOT NULL,
    block_data TEXT NOT NULL);

  CREATE TABLE tbl_commod(
    commod_id INT PRIMARY KEY NOT NULL,
    commod_data TEXT NOT NULL);

  CREATE TABLE tbl_ship(
    ship_id INT PRIMARY KEY NOT NULL,
    ship_data TEXT NOT NULL);
)";
  char* err_msg = nullptr;
  int err = sqlite3_exec(dbconn, tbl_create, nullptr, nullptr, &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQL error: {}", err_msg);
    sqlite3_free(err_msg);
  }
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

void Sql::getsdata(stardata* S) { ::getsdata(S); }
void getsdata(stardata* S) {
  // Read from SQLite database
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql = "SELECT stardata_json FROM tbl_stardata WHERE id = 1";

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
  const char* sql = "SELECT race_data FROM tbl_race WHERE player_id = ?1";

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
  const char* sql = "SELECT star_data FROM tbl_star WHERE star_id = ?1";

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
      "SELECT planet_data FROM tbl_planet WHERE star_id=?1 AND planet_order=?2";
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
      "SELECT sector_data FROM tbl_sector "
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
      "SELECT sector_data FROM tbl_sector "
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
  const char* sql = "SELECT ship_data FROM tbl_ship WHERE ship_id = ?1";

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

      auto ship_opt = ship_from_json(json_string);
      if (ship_opt.has_value()) {
        Ship ship = ship_opt.value();
        
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
  const char* sql = "SELECT commod_data FROM tbl_commod WHERE commod_id = ?1";

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
      LIMIT (SELECT IFNULL(MAX(ship_id), 0) + 1 FROM tbl_ship)
    )
    SELECT x FROM cnt
    WHERE x NOT IN (SELECT ship_id FROM tbl_ship)
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
      LIMIT (SELECT IFNULL(MAX(commod_id), 0) + 1 FROM tbl_commod)
    )
    SELECT x FROM cnt
    WHERE x NOT IN (SELECT commod_id FROM tbl_commod)
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
  const char* sql =
      "REPLACE INTO tbl_stardata (id, stardata_json) VALUES (1, ?1)";

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
  const char* sql =
      "REPLACE INTO tbl_race (player_id, race_data) VALUES (?1, ?2)";

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
  const char* sql =
      "REPLACE INTO tbl_star (star_id, star_data) VALUES (?1, ?2)";

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
      "REPLACE INTO tbl_planet (planet_id, star_id, planet_order, planet_data) "
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
      "REPLACE INTO tbl_sector (planet_id, xpos, ypos, sector_data) "
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
  // Serialize Ship to JSON using existing function
  auto json_result = ship_to_json(s);
  if (!json_result.has_value()) {
    std::println(stderr, "Error: Failed to serialize Ship to JSON");
    return;
  }

  // Store in SQLite database as JSON
  const char* tail;
  sqlite3_stmt* stmt;
  const char* sql =
      "REPLACE INTO tbl_ship (ship_id, ship_data) VALUES (?1, ?2)";

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
  const char* sql =
      "REPLACE INTO tbl_commod (commod_id, commod_data) VALUES (?1, ?2)";

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

  const char* sql = "DELETE FROM tbl_ship WHERE ship_id = ?";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, sql, -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, shipnum);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void makecommoddead(int commodnum) {
  if (commodnum == 0) return;

  const char* sql = "DELETE FROM tbl_commod WHERE commod_id = ?";
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
    const char* sql =
        "REPLACE INTO tbl_power (player_id, power_data) VALUES (?1, ?2);";
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
  const char* sql = "SELECT player_id, power_data FROM tbl_power";
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
    const char* sql =
        "REPLACE INTO tbl_block (player_id, block_data) VALUES (?1, ?2)";

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
  const char* sql =
      "SELECT player_id, block_data FROM tbl_block ORDER BY player_id";

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

// JSON serialization functions for Ship - now with native std::variant support
std::optional<std::string> ship_to_json(const Ship& ship) {
  auto result = glz::write_json(ship);
  if (result.has_value()) {
    return result.value();
  }
  return std::nullopt;
}

std::optional<Ship> ship_from_json(const std::string& json_str) {
  Ship ship{};
  auto result = glz::read_json(ship, json_str);
  if (!result) {
    return ship;
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
