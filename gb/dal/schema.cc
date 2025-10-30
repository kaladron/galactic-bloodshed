// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

#include <cstdio>

import std.compat;

module dallib;

void initialize_schema(Database& db) {
  const char* tbl_create = R"(
      CREATE TABLE tbl_planet(
          id INT PRIMARY KEY NOT NULL,
          star_id INT NOT NULL,
          planet_order INT NOT NULL,
          data TEXT NOT NULL);

  CREATE INDEX star_planet ON tbl_planet (star_id, planet_order);

  CREATE TABLE tbl_sector(
    planet_id INT NOT NULL,
    xpos INT NOT NULL,
    ypos INT NOT NULL,
    data TEXT NOT NULL,
    PRIMARY KEY(planet_id, xpos, ypos));

  CREATE TABLE tbl_star(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_power(
      id INT PRIMARY KEY NOT NULL,
      data TEXT NOT NULL);

  CREATE TABLE tbl_race(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_stardata(
    id INT PRIMARY KEY NOT NULL DEFAULT 1,
    data TEXT NOT NULL);

  CREATE TABLE tbl_block(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_commod(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_ship(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);
)";

  char* err_msg = nullptr;
  int err = sqlite3_exec(db.connection(), tbl_create, nullptr, nullptr, &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQL error: {}", err_msg);
    sqlite3_free(err_msg);
  }
}
