// SPDX-License-Identifier: Apache-2.0

/// \file schema.cc
/// \brief SQLite schema initialization for all persistent game tables.

module;

import std;

module dallib;

void initialize_schema(Database& db) {
  const char* tbl_create = R"(
  CREATE TABLE tbl_star(
    id INT PRIMARY KEY NOT NULL CHECK (id >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)));

  CREATE TABLE tbl_race(
    id INT PRIMARY KEY NOT NULL CHECK (id >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)),
    gov_ship INT GENERATED ALWAYS AS (json_extract(data, '$.Gov_ship')) STORED,
    FOREIGN KEY(gov_ship) REFERENCES tbl_ship(id));

  CREATE TABLE tbl_planet(
    star_id INT NOT NULL CHECK (star_id >= 1),
    planet_order INT NOT NULL CHECK (planet_order >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)),
    slaved_to INT GENERATED ALWAYS AS (json_extract(data, '$.slaved_to')) STORED,
    PRIMARY KEY(star_id, planet_order),
    FOREIGN KEY(star_id) REFERENCES tbl_star(id),
    FOREIGN KEY(slaved_to) REFERENCES tbl_race(id));

  CREATE TABLE tbl_sector(
    star_id INT NOT NULL CHECK (star_id >= 1),
    planet_order INT NOT NULL CHECK (planet_order >= 1),
    xpos INT NOT NULL CHECK (xpos >= 0),
    ypos INT NOT NULL CHECK (ypos >= 0),
    data TEXT NOT NULL CHECK (json_valid(data)),
    owner INT GENERATED ALWAYS AS (json_extract(data, '$.owner')) STORED,
    PRIMARY KEY(star_id, planet_order, xpos, ypos),
    FOREIGN KEY(star_id, planet_order) REFERENCES tbl_planet(star_id, planet_order),
    FOREIGN KEY(owner) REFERENCES tbl_race(id));

  CREATE TABLE tbl_power(
    id INT PRIMARY KEY NOT NULL CHECK (id >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)),
    FOREIGN KEY(id) REFERENCES tbl_race(id));

  CREATE TABLE tbl_universe(
    id INT PRIMARY KEY NOT NULL DEFAULT 1 CHECK (id = 1),
    data TEXT NOT NULL CHECK (json_valid(data)));

  CREATE TABLE tbl_server_state(
    id INT PRIMARY KEY NOT NULL DEFAULT 1 CHECK (id = 1),
    data TEXT NOT NULL CHECK (json_valid(data)));

  CREATE TABLE tbl_block(
    id INT PRIMARY KEY NOT NULL CHECK (id >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)),
    FOREIGN KEY(id) REFERENCES tbl_race(id));

  CREATE TABLE tbl_commod(
    id INTEGER PRIMARY KEY AUTOINCREMENT CHECK (id >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)),
    owner INT NOT NULL GENERATED ALWAYS AS (json_extract(data, '$.owner')) STORED,
    bidder INT GENERATED ALWAYS AS (json_extract(data, '$.bidder')) STORED,
    star_from INT GENERATED ALWAYS AS (json_extract(data, '$.star_from')) STORED,
    planet_from INT GENERATED ALWAYS AS (json_extract(data, '$.planet_from')) STORED,
    star_to INT GENERATED ALWAYS AS (json_extract(data, '$.star_to')) STORED,
    planet_to INT GENERATED ALWAYS AS (json_extract(data, '$.planet_to')) STORED,
    FOREIGN KEY(owner) REFERENCES tbl_race(id),
    FOREIGN KEY(bidder) REFERENCES tbl_race(id),
    FOREIGN KEY(star_from, planet_from) REFERENCES tbl_planet(star_id, planet_order),
    FOREIGN KEY(star_to, planet_to) REFERENCES tbl_planet(star_id, planet_order));

  CREATE TABLE tbl_ship(
    id INTEGER PRIMARY KEY AUTOINCREMENT CHECK (id >= 1),
    data TEXT NOT NULL CHECK (json_valid(data)),
    owner INT NOT NULL GENERATED ALWAYS AS (json_extract(data, '$.owner')) STORED,
    storbits INT GENERATED ALWAYS AS (json_extract(data, '$.storbits')) STORED,
    pnumorbits INT GENERATED ALWAYS AS (json_extract(data, '$.pnumorbits')) STORED,
    whatorbits INT GENERATED ALWAYS AS (json_extract(data, '$.whatorbits')) STORED,
    deststar INT GENERATED ALWAYS AS (json_extract(data, '$.deststar')) STORED,
    destpnum INT GENERATED ALWAYS AS (json_extract(data, '$.destpnum')) STORED,
    whatdest INT GENERATED ALWAYS AS (json_extract(data, '$.whatdest')) STORED,
    destshipno INT GENERATED ALWAYS AS (json_extract(data, '$.destshipno')) STORED,
    alive INT GENERATED ALWAYS AS (json_extract(data, '$.alive')) STORED CHECK (alive = 1),
    protect_ship INT GENERATED ALWAYS AS (json_extract(data, '$.protect.ship')) STORED,
    aimed_shipno INT GENERATED ALWAYS AS (json_extract(data, '$.special.shipno')) STORED,
    aimed_snum INT GENERATED ALWAYS AS (json_extract(data, '$.special.snum')) STORED,
    aimed_pnum INT GENERATED ALWAYS AS (json_extract(data, '$.special.pnum')) STORED,
    mind_target_player INT GENERATED ALWAYS AS (json_extract(data, '$.special.target_player')) STORED,
    mind_who_killed INT GENERATED ALWAYS AS (json_extract(data, '$.special.who_killed')) STORED,
    transport_target_ship INT GENERATED ALWAYS AS (json_extract(data, '$.special.target_ship')) STORED,
    CHECK (
      (whatorbits = 0 AND storbits IS NULL AND pnumorbits IS NULL) OR
      (whatorbits = 1 AND storbits IS NOT NULL AND pnumorbits IS NULL) OR
      (whatorbits = 2 AND storbits IS NOT NULL AND pnumorbits IS NOT NULL) OR
      (whatorbits = 3 AND destshipno IS NOT NULL)
    ),
    CHECK (
      (whatdest = 0 AND deststar IS NULL AND destpnum IS NULL AND (whatorbits = 3 OR destshipno IS NULL)) OR
      (whatdest = 1 AND deststar IS NOT NULL AND destpnum IS NULL) OR
      (whatdest = 2 AND deststar IS NOT NULL AND destpnum IS NOT NULL) OR
      (whatdest = 3 AND destshipno IS NOT NULL)
    ),
    FOREIGN KEY(owner) REFERENCES tbl_race(id),
    FOREIGN KEY(storbits) REFERENCES tbl_star(id),
    FOREIGN KEY(storbits, pnumorbits) REFERENCES tbl_planet(star_id, planet_order),
    FOREIGN KEY(deststar) REFERENCES tbl_star(id),
    FOREIGN KEY(deststar, destpnum) REFERENCES tbl_planet(star_id, planet_order),
    FOREIGN KEY(destshipno) REFERENCES tbl_ship(id),
    FOREIGN KEY(protect_ship) REFERENCES tbl_ship(id),
    FOREIGN KEY(aimed_shipno) REFERENCES tbl_ship(id),
    FOREIGN KEY(aimed_snum) REFERENCES tbl_star(id),
    FOREIGN KEY(aimed_snum, aimed_pnum) REFERENCES tbl_planet(star_id, planet_order),
    FOREIGN KEY(mind_target_player) REFERENCES tbl_race(id),
    FOREIGN KEY(mind_who_killed) REFERENCES tbl_race(id),
    FOREIGN KEY(transport_target_ship) REFERENCES tbl_ship(id));

  CREATE INDEX idx_ship_owner ON tbl_ship(owner);
  CREATE INDEX idx_ship_orbit ON tbl_ship(storbits, pnumorbits, whatorbits);
  CREATE INDEX idx_ship_destship ON tbl_ship(destshipno);
  CREATE INDEX idx_ship_alive ON tbl_ship(alive);

  CREATE TABLE tbl_ship_exam(
    id INT PRIMARY KEY NOT NULL CHECK (id >= 0),
    data TEXT NOT NULL CHECK (json_valid(data)));

  CREATE TABLE tbl_news(
    id INTEGER PRIMARY KEY AUTOINCREMENT CHECK (id >= 1),
    type INT NOT NULL,
    message TEXT NOT NULL,
    timestamp INT NOT NULL);

  CREATE INDEX idx_news_type ON tbl_news(type);
  CREATE INDEX idx_news_timestamp ON tbl_news(type, timestamp);

  CREATE TABLE tbl_telegram(
    id INTEGER PRIMARY KEY AUTOINCREMENT CHECK (id >= 1),
    recipient_player INT NOT NULL CHECK (recipient_player >= 1),
    recipient_governor INT NOT NULL CHECK (recipient_governor >= 1),
    message TEXT NOT NULL,
    timestamp INT NOT NULL,
    FOREIGN KEY(recipient_player) REFERENCES tbl_race(id));

  CREATE INDEX idx_telegram_recipient ON tbl_telegram(recipient_player, recipient_governor);
)";

  db.execute_sql(tbl_create, "Failed to initialize database schema");
}
