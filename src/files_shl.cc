// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

#include "files_shl.h"

#include <fcntl.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "files.h"
#include "files_rw.h"
#include "power.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

static int commoddata, racedata, shdata, stdata;

static sqlite3 *db;

static void start_bulk_insert();
static void end_bulk_insert();

void close_file(int fd) { close(fd); }

void initsqldata() {  // __attribute__((no_sanitize_memory)) {
  const char *tbl_create = R"(
      CREATE TABLE tbl_planet(
          planet_id INT PRIMARY KEY NOT NULL, star_id INT NOT NULL,
          planet_order INT NOT NULL, name TEXT NOT NULL, xpos DOUBLE,
          ypos DOUBLE, ships INT64, Maxx INT, Maxy INT, popn INT64,
          troops INT64, maxpopn INT64, total_resources INT64, slaved_to INT,
          type INT, expltimer INT, condition_rtemp INT, condition_temp INT,
          condition_methane INT, condition_oxygen INT, condition_co2 INT,
          condition_hydrogen INT, condition_nitrogen INT, condition_sulfur INT,
          condition_helium INT, condition_other INT, condition_toxic INT,
          explored INT);

  CREATE INDEX star_planet ON tbl_planet (star_id, planet_order);

  CREATE TABLE
  tbl_sector(planet_id INT NOT NULL, xpos INT NOT NULL, ypos INT NOT NULL,
             eff INT, fert INT, mobilization INT, crystals INT, resource INT,
             popn INT64, troops INT64, owner INT, race INT, type INT,
             condition INT, PRIMARY KEY(planet_id, xpos, ypos));

  CREATE TABLE tbl_plinfo(
      planet_id INT PRIMARY KEY NOT NULL, player_id INT NOT NULL, fuel INT,
      destruct INT, resource INT, popn INT64, troops INT64, crystals INT,
      prod_res INT, prod_fuel INT, prod_dest INT, prod_crystals INT,
      prod_money INT64, prod_tech DOUBLE, tech_invest INT, numsectsowned INT,
      comread INT, mob_set INT, tox_thresh INT, explored INT, autorep INT,
      tax INT, newtax INT, guns INT, mob_points INT64, est_production DOUBLE);

  CREATE TABLE tbl_plinfo_routes(planet_id INT PRIMARY KEY NOT NULL,
                                 player_id INT, routenum INT,
                                 order_set INT, dest_star INT, dest_planet INT,
                                 load INT, unload INT, x INT, y INT);

  CREATE TABLE tbl_star(star_id INT NOT NULL PRIMARY KEY, ships INT, name TEXT,
                        xpos DOUBLE, ypos DOUBLE, numplanets INT, stability INT,
                        nova_stage INT, temperature INT, gravity DOUBLE);

  CREATE TABLE tbl_star_governor(star_id INT NOT NULL, player_id INT NOT NULL,
                                 governor_id INT NOT NULL,
                                 PRIMARY KEY(star_id, player_id));

  CREATE TABLE
  tbl_star_explored(star_id INT NOT NULL, player_id INT NOT NULL, explored INT,
                    PRIMARY KEY(star_id, player_id));

  CREATE TABLE
  tbl_star_inhabited(star_id INT NOT NULL, player_id INT NOT NULL, explored INT,
                     PRIMARY KEY(star_id, player_id));

  CREATE TABLE tbl_star_playerap(star_id INT NOT NULL, player_id INT NOT NULL,
                                 ap INT NOT NULL,
                                 PRIMARY KEY(star_id, player_id));

  CREATE TABLE tbl_stardata(indexnum INT PRIMARY KEY NOT NULL, ships INT);

  CREATE TABLE tbl_stardata_perplayer(
      player_id INT PRIMARY KEY NOT NULL, ap INT NOT NULL,
      VN_hitlist INT NOT NULL, VN_index1 INT NOT NULL, VN_index2 INT NOT NULL);

  CREATE TABLE tbl_ship(
      ship_id INT PRIMARY KEY NOT NULL,
      player_id INT NOT NULL,
      governor_id INT NOT NULL,
      name TEXT NOT NULL,
      shipclass TEXT NOT NULL,
      race INT NOT NULL,
      xpos DOUBLE NOT NULL,
      ypos DOUBLE NOT NULL,
      mass DOUBLE NOT NULL,
      land_x INT,
      land_y INT,
      destshipno INT,
      nextship INT,
      ships INT,
      armor INT,
      size INT,

      max_crew INT,
      max_resource INT,
      max_destruct INT,
      max_fuel INT,
      max_speed INT,
      build_type INT,
      build_cost INT,

      base_mass DOUBLE,
      tech DOUBLE,
      complexity DOUBLE,

      destruct INT,
      resource INT,
      population INT64,
      troops INT64,
      crystals INT,

      aimed_shipno INT,
      aimed_snum INT,
      aimed_intensity INT,
      aimed_pnum INT,
      aimed_level INT,

      mind_progenitor INT,
      mind_target INT,
      mind_generation INT,
      mind_busy INT,
      mind_tampered INT,
      mind_who_killed INT,

      pod_decay INT,
      pod_temperature INT,

      timer_count INT,

      impact_x INT,
      impact_y INT,
      impact_scatter INT,

      trigger_radius INT,

      terraform_index INT,

      transport_target INT,

      waste_toxic INT,

      who_killed INT,

      navigate_on INT,
      navigate_speed INT,
      navigate_turns INT,
      navigate_bearing INT,

      protect_maxrng DOUBLE,
      protect_on INT,
      protect_planet INT,
      protect_self INT,
      protect_evade INT,
      protect_ship INT,

      hyper_drive_charge INT,
      hyper_drive_ready INT,
      hyper_drive_on INT,
      hyper_drive_has INT,

      cew INT,
      cew_range INT,
      cloak INT,
      laser INT,
      focus INT,
      fire_laser INT,
      storbits INT,
      deststar INT,
      destpnum INT,
      pnumorbits INT,
      whatdest INT,
      whatorbits INT,

      damage INT,
      rad INT,
      retaliate INT,
      target INT,

      type INT,
      speed INT,

      active INT,
      alive INT,
      mode INT,
      bombard INT,
      mounted INT,
      cloaked INT,
      sheep INT,
      docked INT,
      notified INT,
      examined INT,
      on_off INT,

      merchant INT,
      guns INT,
      primary_gun INT,
      primtype INT,
      secondary_gun INT,
      sectype INT,

      hanger INT,
      max_hanger INT,
      mount INT);
)";
  char *err_msg = nullptr;
  int err = sqlite3_exec(db, tbl_create, nullptr, nullptr, &err_msg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
}

void opensql() {
  int err = sqlite3_open(PKGSTATEDIR "gb.db", &db);
  if (err) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
}

void open_data_files() {
  opensql();
  opencommoddata(&commoddata);
  openracedata(&racedata);
  openshdata(&shdata);
  openstardata(&stdata);
}

void close_data_files() {
  close_file(commoddata);
  close_file(racedata);
  close_file(shdata);
  close_file(stdata);
}

void openstardata(int *fd) {
  /*printf(" openstardata\n");*/
  if ((*fd = open(STARDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openstardata");
    printf("unable to open %s\n", STARDATAFL);
    exit(-1);
  }
}

void openshdata(int *fd) {
  if ((*fd = open(SHIPDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openshdata");
    printf("unable to open %s\n", SHIPDATAFL);
    exit(-1);
  }
}

void opencommoddata(int *fd) {
  if ((*fd = open(COMMODDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("opencommoddata");
    printf("unable to open %s\n", COMMODDATAFL);
    exit(-1);
  }
}

void openracedata(int *fd) {
  if ((*fd = open(RACEDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openrdata");
    printf("unable to open %s\n", RACEDATAFL);
    exit(-1);
  }
}

void getsdata(struct stardata *S) {
  Fileread(stdata, (char *)S, sizeof(struct stardata), 0);
}

void getrace(racetype **r, int rnum) {
  *r = (racetype *)malloc(sizeof(racetype));
  Fileread(racedata, (char *)*r, sizeof(racetype),
           (rnum - 1) * sizeof(racetype));
}

void getstar(startype **s, int star) {
  if (s >= &Stars[0] && s < &Stars[NUMSTARS])
    ; /* Do nothing */
  else {
    *s = (startype *)malloc(sizeof(startype));
  }
  memset(*s, 0, sizeof(startype));
  Fileread(stdata, (char *)*s, sizeof(startype),
           (int)(sizeof(Sdata) + star * sizeof(startype)));
  const char *tail;

  {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT ships, name, xpos, ypos, "
        "numplanets, stability, nova_stage, temperature, gravity "
        "FROM tbl_star WHERE star_id=?1 LIMIT 1";
    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

    sqlite3_bind_int(stmt, 1, star);
    sqlite3_step(stmt);
    (*s)->ships = static_cast<short>(sqlite3_column_int(stmt, 0));
    strcpy((*s)->name,
           reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
    (*s)->xpos = sqlite3_column_double(stmt, 2);
    (*s)->ypos = sqlite3_column_double(stmt, 3);
    (*s)->numplanets = static_cast<short>(sqlite3_column_int(stmt, 4));
    (*s)->stability = static_cast<short>(sqlite3_column_int(stmt, 5));
    (*s)->nova_stage = static_cast<short>(sqlite3_column_int(stmt, 6));
    (*s)->temperature = static_cast<short>(sqlite3_column_int(stmt, 7));
    (*s)->gravity = sqlite3_column_double(stmt, 8);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
  }
  {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT player_id, governor_id FROM tbl_star_governor "
        "WHERE star_id=?1";
    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

    sqlite3_bind_int(stmt, 1, star);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      player_t p = sqlite3_column_int(stmt, 0);
      (*s)->governor[p - 1] = sqlite3_column_int(stmt, 1);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
  }
}

Planet getplanet(const starnum_t star, const planetnum_t pnum) {
  const char *tail, *plinfo_tail, *plinfo_routes_tail;
  sqlite3_stmt *stmt, *plinfo_stmt, *plinfo_routes_stmt;
  const char *sql =
      "SELECT planet_id, star_id, planet_order, name, "
      "xpos, ypos, ships, maxx, maxy, popn, troops, maxpopn, total_resources, "
      "slaved_to, type, expltimer, condition_rtemp, condition_temp, "
      "condition_methane, condition_oxygen, condition_co2, "
      "condition_hydrogen, condition_nitrogen, condition_sulfur, "
      "condition_helium, condition_other, condition_toxic, "
      "explored FROM tbl_planet WHERE star_id=?1 AND planet_order=?2";
  sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

  sqlite3_bind_int(stmt, 1, star);
  sqlite3_bind_int(stmt, 2, pnum);

  auto result = sqlite3_step(stmt);
  if (result != SQLITE_ROW) {
    throw new std::runtime_error(
        "Database unable to return the requested planet");
  }

  Planet p;
  p.planet_id = sqlite3_column_int(stmt, 0);
  p.xpos = sqlite3_column_double(stmt, 4);
  p.ypos = sqlite3_column_double(stmt, 5);
  p.ships = sqlite3_column_int(stmt, 6);
  p.Maxx = sqlite3_column_int(stmt, 7);
  p.Maxy = sqlite3_column_int(stmt, 8);
  p.popn = sqlite3_column_int(stmt, 9);
  p.troops = sqlite3_column_int(stmt, 10);
  p.maxpopn = sqlite3_column_int(stmt, 11);
  p.total_resources = sqlite3_column_int(stmt, 12);
  p.slaved_to = sqlite3_column_int(stmt, 13);
  int p_type = sqlite3_column_int(stmt, 14);
  switch (p_type) {
    case 0:
      p.type = PlanetType::EARTH;
      break;
    case 1:
      p.type = PlanetType::ASTEROID;
      break;
    case 2:
      p.type = PlanetType::MARS;
      break;
    case 3:
      p.type = PlanetType::ICEBALL;
      break;
    case 4:
      p.type = PlanetType::GASGIANT;
      break;
    case 5:
      p.type = PlanetType::WATER;
      break;
    case 6:
      p.type = PlanetType::FOREST;
      break;
    case 7:
      p.type = PlanetType::DESERT;
      break;
    default:
      throw new std::runtime_error("Bad data in type field");
  }
  p.expltimer = sqlite3_column_int(stmt, 15);
  p.conditions[RTEMP] = sqlite3_column_int(stmt, 16);
  p.conditions[TEMP] = sqlite3_column_int(stmt, 17);
  p.conditions[METHANE] = sqlite3_column_int(stmt, 18);
  p.conditions[OXYGEN] = sqlite3_column_int(stmt, 19);
  p.conditions[CO2] = sqlite3_column_int(stmt, 20);
  p.conditions[HYDROGEN] = sqlite3_column_int(stmt, 21);
  p.conditions[NITROGEN] = sqlite3_column_int(stmt, 22);
  p.conditions[SULFUR] = sqlite3_column_int(stmt, 23);
  p.conditions[HELIUM] = sqlite3_column_int(stmt, 24);
  p.conditions[OTHER] = sqlite3_column_int(stmt, 25);
  p.conditions[TOXIC] = sqlite3_column_int(stmt, 26);

  const char *plinfo_sql =
      "SELECT planet_id, player_id, fuel, destruct, "
      "resource, popn, troops, crystals, prod_res, "
      "prod_fuel, prod_dest, prod_crystals, prod_money, "
      "prod_tech, tech_invest, numsectsowned, comread, "
      "mob_set, tox_thresh, explored, autorep, tax, "
      "newtax, guns, mob_points, est_production FROM tbl_plinfo "
      "WHERE planet_id=?1";
  sqlite3_prepare_v2(db, plinfo_sql, -1, &plinfo_stmt, &plinfo_tail);
  sqlite3_bind_int(plinfo_stmt, 1, p.planet_id);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int player_id = sqlite3_column_int(plinfo_stmt, 1);
    p.info[player_id].fuel = sqlite3_column_int(plinfo_stmt, 2);
    p.info[player_id].destruct = sqlite3_column_int(plinfo_stmt, 3);
    p.info[player_id].resource = sqlite3_column_int(plinfo_stmt, 4);
    p.info[player_id].popn = sqlite3_column_int(plinfo_stmt, 5);
    p.info[player_id].troops = sqlite3_column_int(plinfo_stmt, 6);
    p.info[player_id].crystals = sqlite3_column_int(plinfo_stmt, 7);
    p.info[player_id].prod_res = sqlite3_column_int(plinfo_stmt, 8);
    p.info[player_id].prod_fuel = sqlite3_column_int(plinfo_stmt, 9);
    p.info[player_id].prod_dest = sqlite3_column_int(plinfo_stmt, 10);
    p.info[player_id].prod_crystals = sqlite3_column_int(plinfo_stmt, 11);
    p.info[player_id].prod_money = sqlite3_column_int(plinfo_stmt, 12);
    p.info[player_id].prod_tech = sqlite3_column_int(plinfo_stmt, 13);
    p.info[player_id].tech_invest = sqlite3_column_int(plinfo_stmt, 14);
    p.info[player_id].numsectsowned = sqlite3_column_int(plinfo_stmt, 15);
    p.info[player_id].comread = sqlite3_column_int(plinfo_stmt, 16);
    p.info[player_id].mob_set = sqlite3_column_int(plinfo_stmt, 17);
    p.info[player_id].tox_thresh = sqlite3_column_int(plinfo_stmt, 18);
    p.info[player_id].explored = sqlite3_column_int(plinfo_stmt, 19);
    p.info[player_id].autorep = sqlite3_column_int(plinfo_stmt, 20);
    p.info[player_id].tax = sqlite3_column_int(plinfo_stmt, 21);
    p.info[player_id].newtax = sqlite3_column_int(plinfo_stmt, 22);
    p.info[player_id].guns = sqlite3_column_int(plinfo_stmt, 23);
    p.info[player_id].mob_points = sqlite3_column_int(plinfo_stmt, 24);
    p.info[player_id].est_production = sqlite3_column_int(plinfo_stmt, 25);
  }

  const char *plinfo_routes_sql =
      "SELECT planet_id, player_id, routenum, order_set, dest_star, "
      "dest_planet, load, unload, x, y FROM tbl_plinfo_routes WHERE "
      "planet_id=1";
  sqlite3_prepare_v2(db, plinfo_routes_sql, -1, &plinfo_routes_stmt,
                     &plinfo_routes_tail);
  sqlite3_bind_int(plinfo_routes_stmt, 1, p.planet_id);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int player_id = sqlite3_column_int(plinfo_routes_stmt, 1);
    int routenum = sqlite3_column_int(plinfo_routes_stmt, 2);
    p.info[player_id].route[routenum].set =
        sqlite3_column_int(plinfo_routes_stmt, 3);
    p.info[player_id].route[routenum].dest_star =
        sqlite3_column_int(plinfo_routes_stmt, 4);
    p.info[player_id].route[routenum].dest_planet =
        sqlite3_column_int(plinfo_routes_stmt, 5);
    p.info[player_id].route[routenum].load =
        sqlite3_column_int(plinfo_routes_stmt, 6);
    p.info[player_id].route[routenum].unload =
        sqlite3_column_int(plinfo_routes_stmt, 7);
    p.info[player_id].route[routenum].x =
        sqlite3_column_int(plinfo_routes_stmt, 8);
    p.info[player_id].route[routenum].y =
        sqlite3_column_int(plinfo_routes_stmt, 9);
  }
  return p;
}

sector getsector(const Planet &p, const int x, const int y) {
  const char *tail;
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT planet_id, xpos, ypos, eff, fert, "
      "mobilization, crystals, resource, popn, troops, owner, "
      "race, type, condition FROM tbl_sector "
      "WHERE planet_id=?1 AND xpos=?2 AND ypos=?3";
  sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

  sqlite3_bind_int(stmt, 1, p.planet_id);
  sqlite3_bind_int(stmt, 2, x);
  sqlite3_bind_int(stmt, 3, y);

  auto result = sqlite3_step(stmt);
  if (result != SQLITE_ROW) {
    throw new std::runtime_error(
        "Database unable to return the requested sector");
  }

  sector s(sqlite3_column_int(stmt, 3),   // eff
           sqlite3_column_int(stmt, 4),   // fert
           sqlite3_column_int(stmt, 5),   // mobilization
           sqlite3_column_int(stmt, 6),   // crystals
           sqlite3_column_int(stmt, 7),   // resource
           sqlite3_column_int(stmt, 8),   // popn
           sqlite3_column_int(stmt, 9),   // troops
           sqlite3_column_int(stmt, 10),  // owner
           sqlite3_column_int(stmt, 11),  // race
           sqlite3_column_int(stmt, 12),  // type
           sqlite3_column_int(stmt, 13)   // condition
  );

  int err = sqlite3_finalize(stmt);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQLite Error: %s\n", sqlite3_errmsg(db));
  }

  return s;
}

sector_map getsmap(const Planet &p) {
  const char *tail = nullptr;
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT planet_id, xpos, ypos, eff, fert, "
      "mobilization, crystals, resource, popn, troops, owner, "
      "race, type, condition FROM tbl_sector "
      "WHERE planet_id=?1 ORDER BY ypos, xpos";
  sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

  sqlite3_bind_int(stmt, 1, p.planet_id);

  sector_map smap(p);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    sector s(sqlite3_column_int(stmt, 3),   // eff
             sqlite3_column_int(stmt, 4),   // fert
             sqlite3_column_int(stmt, 5),   // mobilization
             sqlite3_column_int(stmt, 6),   // crystals
             sqlite3_column_int(stmt, 7),   // resource
             sqlite3_column_int(stmt, 8),   // popn
             sqlite3_column_int(stmt, 9),   // troops
             sqlite3_column_int(stmt, 10),  // owner
             sqlite3_column_int(stmt, 11),  // race
             sqlite3_column_int(stmt, 12),  // type
             sqlite3_column_int(stmt, 13)   // condition
    );
    smap.put(std::move(s));
  }

  sqlite3_clear_bindings(stmt);
  sqlite3_reset(stmt);

  return smap;
}

int getship(Ship **s, shipnum_t shipnum) {
  struct stat buffer;

  if (shipnum <= 0) return 0;

  fstat(shdata, &buffer);
  if (buffer.st_size / sizeof(Ship) < shipnum) return 0;

  if ((*s = (Ship *)malloc(sizeof(Ship))) == nullptr) {
    printf("getship:malloc() error \n");
    exit(0);
  }

  Fileread(shdata, (char *)*s, sizeof(Ship), (shipnum - 1) * sizeof(Ship));
  return 1;
}

int getcommod(commodtype **c, commodnum_t commodnum) {
  struct stat buffer;

  if (commodnum <= 0) return 0;

  fstat(commoddata, &buffer);
  if (buffer.st_size / sizeof(commodtype) < commodnum) return 0;

  if ((*c = (commodtype *)malloc(sizeof(commodtype))) == nullptr) {
    printf("getcommod:malloc() error \n");
    exit(0);
  }

  Fileread(commoddata, (char *)*c, sizeof(commodtype),
           (commodnum - 1) * sizeof(commodtype));
  return 1;
}

/* gets the ship # listed in the top of the file SHIPFREEDATAFL. this
** might have no other uses besides build().
*/
int getdeadship() {
  struct stat buffer;
  short shnum;
  int fd;
  int abort;

  if ((fd = open(SHIPFREEDATAFL, O_RDWR, 0777)) < 0) {
    perror("getdeadship");
    printf("unable to open %s\n", SHIPFREEDATAFL);
    exit(-1);
  }
  abort = 1;
  fstat(fd, &buffer);

  if (buffer.st_size && (abort == 1)) {
    /* put topmost entry in fpos */
    Fileread(fd, (char *)&shnum, sizeof(short), buffer.st_size - sizeof(short));
    /* erase that entry, since it will now be filled */
    ftruncate(fd, (long)(buffer.st_size - sizeof(short)));
    close_file(fd);
    return (int)shnum;
  }
  close_file(fd);
  return -1;
}

int getdeadcommod() {
  struct stat buffer;
  short commodnum;
  int fd;
  int abort;

  if ((fd = open(COMMODFREEDATAFL, O_RDWR, 0777)) < 0) {
    perror("getdeadcommod");
    printf("unable to open %s\n", COMMODFREEDATAFL);
    exit(-1);
  }
  abort = 1;
  fstat(fd, &buffer);

  if (buffer.st_size && (abort == 1)) {
    /* put topmost entry in fpos */
    Fileread(fd, (char *)&commodnum, sizeof(short),
             buffer.st_size - sizeof(short));
    /* erase that entry, since it will now be filled */
    ftruncate(fd, (long)(buffer.st_size - sizeof(short)));
    close_file(fd);
    return (int)commodnum;
  }
  close_file(fd);
  return -1;
}

void putsdata(struct stardata *S) {
  Filewrite(stdata, (char *)S, sizeof(struct stardata), 0);
}

void putrace(racetype *r) {
  Filewrite(racedata, (char *)r, sizeof(racetype),
            (r->Playernum - 1) * sizeof(racetype));
}

void putstar(startype *s, starnum_t snum) {
  Filewrite(stdata, (char *)s, sizeof(startype),
            (int)(sizeof(Sdata) + snum * sizeof(startype)));

  start_bulk_insert();

  {
    const char *tail = nullptr;
    sqlite3_stmt *stmt;

    const char *sql =
        "REPLACE INTO tbl_star (star_id, ships, name, xpos, ypos, "
        "numplanets, stability, nova_stage, temperature, gravity) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

    sqlite3_bind_int(stmt, 1, snum);
    sqlite3_bind_int(stmt, 2, s->ships);
    sqlite3_bind_text(stmt, 3, s->name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, s->xpos);
    sqlite3_bind_double(stmt, 5, s->ypos);
    sqlite3_bind_int(stmt, 6, s->numplanets);
    sqlite3_bind_int(stmt, 7, s->stability);
    sqlite3_bind_int(stmt, 8, s->nova_stage);
    sqlite3_bind_int(stmt, 9, s->temperature);
    sqlite3_bind_double(stmt, 10, s->gravity);

    sqlite3_step(stmt);

    sqlite3_reset(stmt);
  }

  {
    const char *tail = nullptr;
    sqlite3_stmt *stmt;
    const char *sql =
        "REPLACE INTO tbl_star_governor (star_id, player_id, governor_id) "
        "VALUES (?1, ?2, ?3)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    for (player_t i = 1; i <= MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, s->governor[i - 1]);

      sqlite3_step(stmt);

      sqlite3_reset(stmt);
    }
  }

  {
    const char *tail = nullptr;
    sqlite3_stmt *stmt;
    const char *sql =
        "REPLACE INTO tbl_star_playerap (star_id, player_id, ap) "
        "VALUES (?1, ?2, ?3)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    for (player_t i = 1; i <= MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, s->AP[i - 1]);

      sqlite3_step(stmt);

      sqlite3_reset(stmt);
    }
  }

  {
    const char *tail = nullptr;
    sqlite3_stmt *stmt;
    const char *sql =
        "REPLACE INTO tbl_star_explored (star_id, player_id, explored) "
        "VALUES (?1, ?2, ?3)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    for (player_t i = 1; i <= MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, isset(s->explored, i - 1) ? 1 : 0);

      sqlite3_step(stmt);

      sqlite3_reset(stmt);
    }
  }

  {
    const char *tail = nullptr;
    sqlite3_stmt *stmt;
    const char *sql =
        "REPLACE INTO tbl_star_inhabited (star_id, player_id, explored) "
        "VALUES (?1, ?2, ?3)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    for (player_t i = 1; i <= MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, isset(s->inhabited, i - 1) ? 1 : 0);

      sqlite3_step(stmt);

      sqlite3_reset(stmt);
    }
  }

  end_bulk_insert();
}

static void start_bulk_insert() {
  char *err_msg = nullptr;
  sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, &err_msg);
}

static void end_bulk_insert() {
  char *err_msg = nullptr;
  sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, &err_msg);
}

void putplanet(const Planet &p, startype *star, const int pnum) {
  start_bulk_insert();

  const char *tail = nullptr;
  const char *plinfo_tail = nullptr;
  const char *plinfo_route_tail = nullptr;
  sqlite3_stmt *stmt, *plinfo_stmt, *plinfo_route_stmt;
  const char *sql =
      "REPLACE INTO tbl_planet (planet_id, star_id, planet_order, name, "
      "xpos, ypos, ships, maxx, maxy, popn, troops, maxpopn, total_resources, "
      "slaved_to, type, expltimer, condition_rtemp, condition_temp, "
      "condition_methane, condition_oxygen, condition_co2, "
      "condition_hydrogen, condition_nitrogen, condition_sulfur, "
      "condition_helium, condition_other, condition_toxic, "
      "explored) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, "
      "?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, "
      "?21, ?22, ?23, ?24, ?25, ?26, ?27, ?28)";
  sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);

  const char *plinfo_sql =
      "REPLACE INTO tbl_plinfo (planet_id, player_id, fuel, destruct, "
      "resource, popn, troops, crystals, prod_res, "
      "prod_fuel, prod_dest, prod_crystals, prod_money, "
      "prod_tech, tech_invest, numsectsowned, comread, "
      "mob_set, tox_thresh, explored, autorep, tax, "
      "newtax, guns, mob_points, est_production) VALUES "
      "(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, "
      "?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, "
      "?21, ?22, ?23, ?24, ?25, ?26)";
  if (sqlite3_prepare_v2(db, plinfo_sql, -1, &plinfo_stmt, &plinfo_tail) !=
      SQLITE_OK) {
    fprintf(stderr, "PLINFO %s\n", sqlite3_errmsg(db));
  }

  const char *plinfo_route_sql =
      "REPLACE INTO tbl_plinfo_routes (planet_id, player_id, routenum, "
      "order_set, dest_star, dest_planet, "
      "load, unload, x, y) VALUES "
      "(?1, ?2, ?3, ?4, 5, ?6, ?7, ?8, ?9, ?10)";
  sqlite3_prepare_v2(db, plinfo_route_sql, -1, &plinfo_route_stmt,
                     &plinfo_route_tail);

  sqlite3_bind_int(stmt, 1, p.planet_id);
  sqlite3_bind_int(stmt, 2, star->star_id);
  sqlite3_bind_int(stmt, 3, pnum);
  sqlite3_bind_text(stmt, 4, star->pnames[pnum], strlen(star->pnames[pnum]),
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 5, p.xpos);
  sqlite3_bind_double(stmt, 6, p.ypos);
  sqlite3_bind_int(stmt, 7, p.ships);
  sqlite3_bind_int(stmt, 8, p.Maxx);
  sqlite3_bind_int(stmt, 9, p.Maxy);
  sqlite3_bind_int(stmt, 10, p.popn);
  sqlite3_bind_int(stmt, 11, p.troops);
  sqlite3_bind_int(stmt, 12, p.maxpopn);
  sqlite3_bind_int(stmt, 13, p.total_resources);
  sqlite3_bind_int(stmt, 14, p.slaved_to);
  sqlite3_bind_int(stmt, 15, p.type);
  sqlite3_bind_int(stmt, 16, p.expltimer);
  sqlite3_bind_int(stmt, 17, p.conditions[RTEMP]);
  sqlite3_bind_int(stmt, 18, p.conditions[TEMP]);
  sqlite3_bind_int(stmt, 19, p.conditions[METHANE]);
  sqlite3_bind_int(stmt, 20, p.conditions[OXYGEN]);
  sqlite3_bind_int(stmt, 21, p.conditions[CO2]);
  sqlite3_bind_int(stmt, 22, p.conditions[HYDROGEN]);
  sqlite3_bind_int(stmt, 23, p.conditions[NITROGEN]);
  sqlite3_bind_int(stmt, 24, p.conditions[SULFUR]);
  sqlite3_bind_int(stmt, 25, p.conditions[HELIUM]);
  sqlite3_bind_int(stmt, 26, p.conditions[OTHER]);
  sqlite3_bind_int(stmt, 27, p.conditions[TOXIC]);
  sqlite3_bind_int(stmt, 28, p.explored);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    fprintf(stderr, "XXX %s\n", sqlite3_errmsg(db));
  }

  {
    for (player_t i = 0; i < MAXPLAYERS; i++) {
      sqlite3_bind_int(plinfo_stmt, 1, p.planet_id);
      sqlite3_bind_int(plinfo_stmt, 2, i);
      sqlite3_bind_int(plinfo_stmt, 3, p.info[i].fuel);
      sqlite3_bind_int(plinfo_stmt, 4, p.info[i].destruct);
      sqlite3_bind_int(plinfo_stmt, 5, p.info[i].resource);
      sqlite3_bind_int(plinfo_stmt, 6, p.info[i].popn);
      sqlite3_bind_int(plinfo_stmt, 7, p.info[i].troops);
      sqlite3_bind_int(plinfo_stmt, 8, p.info[i].crystals);
      sqlite3_bind_int(plinfo_stmt, 9, p.info[i].prod_res);
      sqlite3_bind_int(plinfo_stmt, 10, p.info[i].prod_fuel);
      sqlite3_bind_int(plinfo_stmt, 11, p.info[i].prod_dest);
      sqlite3_bind_int(plinfo_stmt, 12, p.info[i].prod_crystals);
      sqlite3_bind_int(plinfo_stmt, 13, p.info[i].prod_money);
      sqlite3_bind_double(plinfo_stmt, 14, p.info[i].prod_tech);
      sqlite3_bind_int(plinfo_stmt, 15, p.info[i].tech_invest);
      sqlite3_bind_int(plinfo_stmt, 16, p.info[i].numsectsowned);
      sqlite3_bind_int(plinfo_stmt, 17, p.info[i].comread);
      sqlite3_bind_int(plinfo_stmt, 18, p.info[i].mob_set);
      sqlite3_bind_int(plinfo_stmt, 19, p.info[i].tox_thresh);
      sqlite3_bind_int(plinfo_stmt, 20, p.info[i].explored);
      sqlite3_bind_int(plinfo_stmt, 21, p.info[i].autorep);
      sqlite3_bind_int(plinfo_stmt, 22, p.info[i].tax);
      sqlite3_bind_int(plinfo_stmt, 23, p.info[i].newtax);
      sqlite3_bind_int(plinfo_stmt, 24, p.info[i].guns);
      sqlite3_bind_int(plinfo_stmt, 25, p.info[i].mob_points);
      sqlite3_bind_double(plinfo_stmt, 26, p.info[i].est_production);

      if (sqlite3_step(plinfo_stmt) != SQLITE_DONE) {
        fprintf(stderr, "YYY %s\n", sqlite3_errmsg(db));
      }
      sqlite3_reset(plinfo_stmt);

      {
        for (int j = 0; j < MAX_ROUTES; j++) {
          sqlite3_bind_int(plinfo_route_stmt, 1, p.planet_id);
          sqlite3_bind_int(plinfo_route_stmt, 2, i);
          sqlite3_bind_int(plinfo_route_stmt, 3, j);
          sqlite3_bind_int(plinfo_route_stmt, 4, p.info[i].route[j].set);
          sqlite3_bind_int(plinfo_route_stmt, 5, p.info[i].route[j].dest_star);
          sqlite3_bind_int(plinfo_route_stmt, 6,
                           p.info[i].route[j].dest_planet);
          sqlite3_bind_int(plinfo_route_stmt, 7, p.info[i].route[j].load);
          sqlite3_bind_int(plinfo_route_stmt, 8, p.info[i].route[j].unload);
          sqlite3_bind_int(plinfo_route_stmt, 9, p.info[i].route[j].x);
          sqlite3_bind_int(plinfo_route_stmt, 10, p.info[i].route[j].y);

          if (sqlite3_step(plinfo_route_stmt) != SQLITE_DONE) {
            fprintf(stderr, "ZZZ %s\n", sqlite3_errmsg(db));
          }
          sqlite3_reset(plinfo_route_stmt);
        }
      }
    }
  }
  sqlite3_finalize(stmt);
  sqlite3_finalize(plinfo_stmt);
  sqlite3_finalize(plinfo_route_stmt);

  end_bulk_insert();
}

void putsector(const sector &s, const Planet &p, const int x, const int y) {
  const char *tail = nullptr;
  sqlite3_stmt *stmt;
  const char *sql =
      "REPLACE INTO tbl_sector (planet_id, xpos, ypos, eff, fert, "
      "mobilization, crystals, resource, popn, troops, owner, "
      "race, type, condition) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14)";

  sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, p.planet_id);
  sqlite3_bind_int(stmt, 2, x);
  sqlite3_bind_int(stmt, 3, y);
  sqlite3_bind_int(stmt, 4, s.eff);
  sqlite3_bind_int(stmt, 5, s.fert);
  sqlite3_bind_int(stmt, 6, s.mobilization);
  sqlite3_bind_int(stmt, 7, s.crystals);
  sqlite3_bind_int(stmt, 8, s.resource);
  sqlite3_bind_int(stmt, 9, s.popn);
  sqlite3_bind_int(stmt, 10, s.troops);
  sqlite3_bind_int(stmt, 11, s.owner);
  sqlite3_bind_int(stmt, 12, s.race);
  sqlite3_bind_int(stmt, 13, s.type);
  sqlite3_bind_int(stmt, 14, s.condition);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    fprintf(stderr, "000 %s\n", sqlite3_errmsg(db));
  }

  sqlite3_reset(stmt);
}

void putsmap(sector_map &map, Planet &p) {
  start_bulk_insert();

  for (int y = 0; y < p.Maxy; y++) {
    for (int x = 0; x < p.Maxx; x++) {
      auto &sec = map.get(x, y);
      putsector(sec, p, x, y);
    }
  }

  end_bulk_insert();
}

void putship(Ship *s) {
  const char *tail;
  Filewrite(shdata, (char *)s, sizeof(Ship), (s->number - 1) * sizeof(Ship));
  start_bulk_insert();

  sqlite3_stmt *stmt;
  const char *sql =
      "REPLACE INTO tbl_ship (ship_id, player_id, governor_id, name, "
      "shipclass, race, xpos, ypos, mass,"
      "land_x, land_y, destshipno, nextship, ships, armor, size,"
      "max_crew, max_resource, max_destruct, max_fuel, max_speed, build_type,"
      "build_cost, base_mass, tech, complexity,"
      "destruct, resource, population, troops, crystals,"
      "who_killed,"
      "navigate_on, navigate_speed, navigate_turns, navigate_bearing,"
      "protect_maxrng, protect_on, protect_planet, protect_self,"
      "protect_evade, protect_ship,"
      "hyper_drive_charge, hyper_drive_ready, hyper_drive_on,"
      "hyper_drive_has,"
      "cew, cew_range, cloak, laser, focus, fire_laser,"
      "storbits, deststar, destpnum, pnumorbits, whatdest,"
      "whatorbits,"
      "damage, rad, retaliate, target,"
      "type, speed,"
      "active, alive, mode, bombard, mounted, cloaked,"
      "sheep, docked, notified, examined, on_off,"
      "merchant, guns, primary_gun, primtype,"
      "secondary_gun, sectype,"
      "hanger, max_hanger, mount)"
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9,"
      "?10, ?11, ?12, ?13, ?14, ?15, ?16,"
      "?17, ?18, ?19, ?20, ?21, ?22, ?23, ?24, ?25, ?26,"
      "?27, ?28, ?29, ?30, ?31,"
      "?32,"
      "?33, ?34, ?35, ?36,"
      "?37, ?38, ?39, ?40,"
      "?41, ?42,"
      "?43, ?44, ?45,"
      "?46,"
      "?47, ?48, ?49, ?50, ?51, ?52,"
      "?53, ?54, ?55, ?56, ?57,"
      "?58,"
      "?59, ?60, ?61, ?62,"
      "?63, ?64,"
      "?65, ?66, ?67, ?68, ?69, ?70,"
      "?71, ?72, ?73, ?74, ?75,"
      "?76, ?77, ?78, ?79,"
      "?80, ?81,"
      "?82, ?83, ?84);";

  sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
  sqlite3_bind_int(stmt, 1, s->number);
  sqlite3_bind_int(stmt, 2, s->owner);
  sqlite3_bind_int(stmt, 3, s->governor);
  sqlite3_bind_text(stmt, 4, s->name, strlen(s->name), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, s->shipclass, strlen(s->shipclass),
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 6, s->race);
  sqlite3_bind_double(stmt, 7, s->xpos);
  sqlite3_bind_double(stmt, 8, s->ypos);
  sqlite3_bind_double(stmt, 9, s->mass);
  sqlite3_bind_int(stmt, 10, s->land_x);
  sqlite3_bind_int(stmt, 11, s->land_y);
  sqlite3_bind_int(stmt, 12, s->destshipno);
  sqlite3_bind_int(stmt, 13, s->nextship);
  sqlite3_bind_int(stmt, 14, s->ships);
  sqlite3_bind_int(stmt, 15, s->armor);
  sqlite3_bind_int(stmt, 16, s->size);
  sqlite3_bind_int(stmt, 17, s->max_crew);
  sqlite3_bind_int(stmt, 18, s->max_resource);
  sqlite3_bind_int(stmt, 19, s->max_destruct);
  sqlite3_bind_int(stmt, 20, s->max_fuel);
  sqlite3_bind_int(stmt, 21, s->max_speed);
  sqlite3_bind_int(stmt, 22, s->build_type);
  sqlite3_bind_int(stmt, 23, s->build_cost);
  sqlite3_bind_double(stmt, 24, s->base_mass);
  sqlite3_bind_double(stmt, 25, s->tech);
  sqlite3_bind_double(stmt, 26, s->complexity);
  sqlite3_bind_int(stmt, 27, s->destruct);
  sqlite3_bind_int(stmt, 28, s->resource);
  sqlite3_bind_int(stmt, 29, s->popn);
  sqlite3_bind_int(stmt, 30, s->troops);
  sqlite3_bind_int(stmt, 31, s->crystals);
  sqlite3_bind_int(stmt, 32, s->who_killed);
  sqlite3_bind_int(stmt, 33, s->navigate.on);
  sqlite3_bind_int(stmt, 34, s->navigate.speed);
  sqlite3_bind_int(stmt, 35, s->navigate.turns);
  sqlite3_bind_int(stmt, 36, s->navigate.bearing);
  sqlite3_bind_double(stmt, 37, s->protect.maxrng);
  sqlite3_bind_int(stmt, 38, s->protect.on);
  sqlite3_bind_int(stmt, 39, s->protect.planet);
  sqlite3_bind_int(stmt, 40, s->protect.self);
  sqlite3_bind_int(stmt, 41, s->protect.evade);
  sqlite3_bind_int(stmt, 42, s->protect.ship);
  sqlite3_bind_int(stmt, 43, s->hyper_drive.charge);
  sqlite3_bind_int(stmt, 44, s->hyper_drive.ready);
  sqlite3_bind_int(stmt, 45, s->hyper_drive.on);
  sqlite3_bind_int(stmt, 46, s->hyper_drive.has);
  sqlite3_bind_int(stmt, 47, s->cew);
  sqlite3_bind_int(stmt, 48, s->cew_range);
  sqlite3_bind_int(stmt, 49, s->cloak);
  sqlite3_bind_int(stmt, 50, s->laser);
  sqlite3_bind_int(stmt, 51, s->focus);
  sqlite3_bind_int(stmt, 52, s->fire_laser);
  sqlite3_bind_int(stmt, 53, s->storbits);
  sqlite3_bind_int(stmt, 54, s->deststar);
  sqlite3_bind_int(stmt, 55, s->destpnum);
  sqlite3_bind_int(stmt, 56, s->pnumorbits);
  sqlite3_bind_int(stmt, 57, s->whatdest);
  sqlite3_bind_int(stmt, 58, s->whatorbits);
  sqlite3_bind_int(stmt, 59, s->damage);
  sqlite3_bind_int(stmt, 60, s->rad);
  sqlite3_bind_int(stmt, 61, s->retaliate);
  sqlite3_bind_int(stmt, 62, s->target);
  sqlite3_bind_int(stmt, 63, s->type);
  sqlite3_bind_int(stmt, 64, s->speed);
  sqlite3_bind_int(stmt, 65, s->active);
  sqlite3_bind_int(stmt, 66, s->alive);
  sqlite3_bind_int(stmt, 67, s->mode);
  sqlite3_bind_int(stmt, 68, s->bombard);
  sqlite3_bind_int(stmt, 69, s->mounted);
  sqlite3_bind_int(stmt, 70, s->cloaked);
  sqlite3_bind_int(stmt, 71, s->sheep);
  sqlite3_bind_int(stmt, 72, s->docked);
  sqlite3_bind_int(stmt, 73, s->notified);
  sqlite3_bind_int(stmt, 74, s->examined);
  sqlite3_bind_int(stmt, 75, s->on);
  sqlite3_bind_int(stmt, 76, s->merchant);
  sqlite3_bind_int(stmt, 77, s->guns);
  sqlite3_bind_int(stmt, 78, s->primary);
  sqlite3_bind_int(stmt, 79, s->primtype);
  sqlite3_bind_int(stmt, 80, s->secondary);
  sqlite3_bind_int(stmt, 81, s->sectype);
  sqlite3_bind_int(stmt, 82, s->hanger);
  sqlite3_bind_int(stmt, 83, s->max_hanger);
  sqlite3_bind_int(stmt, 84, s->mount);

#if 0
  // These are the members of the union.  We'll emit these conditional
  // on ship type
  ShipType::STYPE_MIRROR
      "aimed_shipno, aimed_snum, aimed_intensity, aimed_pnum, aimed_level,"
  ShipType::OTYPE_BERS, ShipType::OTYPE_VN
      "mind_progenitor, mind_target, mind_generation, mind_busy, mind_tampered,"
      "mind_who_killed,"
  ShipType::STYPE_POD
      "pod_decay, pod_temperature,"
  ShipType::OTYPE_CANIST, ShipType::OTYPE_GREEN
      "timer_count,"
  ShipType::STYPE_MISSILE
      "impact_x, impact_y, impact_scatter,"
  ShipType::STYPE_MINE
      "trigger_radius,"
  ShipType::OTYPE_TERRA, ShipType::OTYPE_PLOW
      "terraform_index,"
  ShipType::OTYPE_TRANSDEV
      "transport_target,"
  ShipType::OTYPE_TOXWC
      "waste_toxic,";
#endif

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    fprintf(stderr, "XXX %s\n", sqlite3_errmsg(db));
  }

  int err = sqlite3_finalize(stmt);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQLite Error: %s\n", sqlite3_errmsg(db));
  }

  end_bulk_insert();
}

void putcommod(commodtype *c, int commodnum) {
  Filewrite(commoddata, (char *)c, sizeof(commodtype),
            (commodnum - 1) * sizeof(commodtype));
}

int Numraces() {
  struct stat buffer;

  fstat(racedata, &buffer);
  return ((int)(buffer.st_size / sizeof(racetype)));
}

shipnum_t Numships() /* return number of ships */
{
  struct stat buffer;

  fstat(shdata, &buffer);
  return ((int)(buffer.st_size / sizeof(Ship)));
}

int Numcommods() {
  struct stat buffer;

  fstat(commoddata, &buffer);
  return ((int)(buffer.st_size / sizeof(commodtype)));
}

int Newslength(int type) {
  struct stat buffer;
  FILE *fp;

  switch (type) {
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
  return ((int)buffer.st_size);
}

/* delete contents of dead ship file */
void clr_shipfree() { fclose(fopen(SHIPFREEDATAFL, "w+")); }

void clr_commodfree() { fclose(fopen(COMMODFREEDATAFL, "w+")); }

/*
** writes the ship to the dead ship file at its end.
*/
void makeshipdead(int shipnum) {
  int fd;
  unsigned short shipno;
  struct stat buffer;

  shipno = shipnum; /* conv to u_short */

  if (shipno == 0) return;

  if ((fd = open(SHIPFREEDATAFL, O_WRONLY, 0777)) < 0) {
    printf("fd = %d \n", fd);
    printf("errno = %d \n", errno);
    perror("openshfdata");
    printf("unable to open %s\n", SHIPFREEDATAFL);
    exit(-1);
  }

  /* write the ship # at the very end of SHIPFREEDATAFL */
  fstat(fd, &buffer);

  Filewrite(fd, (char *)&shipno, sizeof(shipno), buffer.st_size);
  close_file(fd);
}

void makecommoddead(int commodnum) {
  int fd;
  unsigned short commodno;
  struct stat buffer;

  commodno = commodnum; /* conv to u_short */

  if (commodno == 0) return;

  if ((fd = open(COMMODFREEDATAFL, O_WRONLY, 0777)) < 0) {
    printf("fd = %d \n", fd);
    printf("errno = %d \n", errno);
    perror("opencommodfdata");
    printf("unable to open %s\n", COMMODFREEDATAFL);
    exit(-1);
  }

  /* write the commod # at the very end of COMMODFREEDATAFL */
  fstat(fd, &buffer);

  Filewrite(fd, (char *)&commodno, sizeof(commodno), buffer.st_size);
  close_file(fd);
}

void Putpower(struct power p[MAXPLAYERS]) {
  int power_fd;

  if ((power_fd = open(POWFL, O_RDWR, 0777)) < 0) {
    perror("open power data");
    printf("unable to open %s\n", POWFL);
    return;
  }
  write(power_fd, (char *)p, sizeof(*p) * MAXPLAYERS);
  close_file(power_fd);
}

void Getpower(struct power p[MAXPLAYERS]) {
  int power_fd;

  if ((power_fd = open(POWFL, O_RDONLY, 0777)) < 0) {
    perror("open power data");
    printf("unable to open %s\n", POWFL);
    return;
  }
  read(power_fd, (char *)p, sizeof(*p) * MAXPLAYERS);
  close_file(power_fd);
}

void Putblock(struct block b[MAXPLAYERS]) {
  int block_fd;

  if ((block_fd = open(BLOCKDATAFL, O_RDWR, 0777)) < 0) {
    perror("open block data");
    printf("unable to open %s\n", BLOCKDATAFL);
    return;
  }
  write(block_fd, (char *)b, sizeof(*b) * MAXPLAYERS);
  close_file(block_fd);
}

void Getblock(struct block b[MAXPLAYERS]) {
  int block_fd;

  if ((block_fd = open(BLOCKDATAFL, O_RDONLY, 0777)) < 0) {
    perror("open block data");
    printf("unable to open %s\n", BLOCKDATAFL);
    return;
  }
  read(block_fd, (char *)b, sizeof(*b) * MAXPLAYERS);
  close_file(block_fd);
}
