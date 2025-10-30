// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

#include <cstdio>

import std.compat;

module dallib;

void initialize_schema(Database& db) {
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
  int err = sqlite3_exec(db.connection(), tbl_create, nullptr, nullptr, &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQL error: {}", err_msg);
    sqlite3_free(err_msg);
  }
}
