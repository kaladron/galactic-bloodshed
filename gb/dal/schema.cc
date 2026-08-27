// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

#include <cstdio>

import std;
#undef stdout

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
    data TEXT NOT NULL,
    owner INT GENERATED ALWAYS AS (json_extract(data, '$.owner')) STORED,
    storbits INT GENERATED ALWAYS AS (json_extract(data, '$.storbits')) STORED,
    pnumorbits INT GENERATED ALWAYS AS (json_extract(data, '$.pnumorbits')) STORED,
    whatorbits INT GENERATED ALWAYS AS (json_extract(data, '$.whatorbits')) STORED,
    destshipno INT GENERATED ALWAYS AS (json_extract(data, '$.destshipno')) STORED,
    alive INT GENERATED ALWAYS AS (json_extract(data, '$.alive')) STORED);

  CREATE INDEX idx_ship_owner ON tbl_ship(owner);
  CREATE INDEX idx_ship_orbit ON tbl_ship(storbits, pnumorbits, whatorbits);
  CREATE INDEX idx_ship_destship ON tbl_ship(destshipno);
  CREATE INDEX idx_ship_alive ON tbl_ship(alive);

  CREATE TABLE tbl_ship_exam(
    id INT PRIMARY KEY NOT NULL,
    data TEXT NOT NULL);

  CREATE TABLE tbl_news(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type INT NOT NULL,
    message TEXT NOT NULL,
    timestamp INT NOT NULL);

  CREATE INDEX idx_news_type ON tbl_news(type);
  CREATE INDEX idx_news_timestamp ON tbl_news(type, timestamp);

  CREATE TABLE tbl_telegram(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    recipient_player INT NOT NULL,
    recipient_governor INT NOT NULL,
    message TEXT NOT NULL,
    timestamp INT NOT NULL);

  CREATE INDEX idx_telegram_recipient ON tbl_telegram(recipient_player, recipient_governor);
)";

  char* raw_err = nullptr;
  int err =
      sqlite3_exec(db.connection(), tbl_create, nullptr, nullptr, &raw_err);
  std::unique_ptr<char, decltype(&sqlite3_free)> err_msg(raw_err, sqlite3_free);
  if (err != SQLITE_OK) {
    throw SqliteError(std::format("Failed to initialize database schema: {}",
                                  err_msg ? err_msg.get() : "Unknown error"),
                      err);
  }
}
