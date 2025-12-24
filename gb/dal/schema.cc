// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

#include <cstdio>

import std.compat;

module dallib;

void initialize_schema(Database& db) {
  const char* tbl_create = R"(
      CREATE TABLE tbl_planet(
          star_id INT NOT NULL,
          planet_order INT NOT NULL,
          data TEXT NOT NULL,
          PRIMARY KEY(star_id, planet_order));

  CREATE TABLE tbl_sector(
    star_id INT NOT NULL,
    planet_order INT NOT NULL,
    xpos INT NOT NULL,
    ypos INT NOT NULL,
    data TEXT NOT NULL,
    PRIMARY KEY(star_id, planet_order, xpos, ypos));

  CREATE TABLE tbl_star(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_power(
      id INT PRIMARY KEY NOT NULL,
      data TEXT NOT NULL);

  CREATE TABLE tbl_race(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_universe(
    id INT PRIMARY KEY NOT NULL DEFAULT 1,
    data TEXT NOT NULL);

  CREATE TABLE tbl_server_state(
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

  CREATE TABLE tbl_news(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type INT NOT NULL,
    message TEXT NOT NULL,
    timestamp INT NOT NULL);

  CREATE INDEX idx_news_type ON tbl_news(type);
  CREATE INDEX idx_news_timestamp ON tbl_news(type, timestamp);
)";

  char* err_msg = nullptr;
  int err =
      sqlite3_exec(db.connection(), tbl_create, nullptr, nullptr, &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQL error: {}", err_msg);
    sqlite3_free(err_msg);
  }
}
