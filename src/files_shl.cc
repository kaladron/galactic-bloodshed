// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

#include "files_shl.h"

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sqlite3.h>
#include <stdio.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include "files.h"
#include "files_rw.h"
#include "power.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

static int commoddata, pdata, racedata, shdata, stdata;

static sqlite3 *db;

static void start_bulk_insert();
static void end_bulk_insert();

void close_file(int fd) { close(fd); }

void initsqldata() __attribute__((no_sanitize_memory)) {
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
      planet_id INT NOT NULL, player_id INT NOT NULL, fuel INT, destruct INT,
      resource INT, popn INT64, troops INT64, crystals INT, prod_res INT,
      prod_fuel INT, prod_dest INT, prod_crystals INT, prod_money INT64,
      prod_tech DOUBLE, tech_invest INT, numsectsowned INT, comread INT,
      mob_set INT, tox_thresh INT, explored INT, autorep INT, tax INT,
      newtax INT, guns INT, mob_points INT64, est_production DOUBLE,
      PRIMARY KEY(planet_id, player_id));

  CREATE TABLE tbl_plinfo_routes(planet_id INT, player_id INT, routenum INT,
                                 order_set INT, dest_star INT, dest_planet INT,
                                 load INT, unload INT, x INT, y INT,
                                 PRIMARY KEY(planet_id, player_id, routenum));

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

)";
  char *err_msg = 0;
  int err = sqlite3_exec(db, tbl_create, NULL, NULL, &err_msg);
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

void open_data_files(void) {
  opensql();
  opencommoddata(&commoddata);
  openpdata(&pdata);
  openracedata(&racedata);
  openshdata(&shdata);
  openstardata(&stdata);
}

void close_data_files(void) {
  close_file(commoddata);
  close_file(pdata);
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

void openpdata(int *fd) {
  if ((*fd = open(PLANETDATAFL, O_RDWR | O_CREAT, 0777)) < 0) {
    perror("openpdata");
    printf("unable to open %s\n", PLANETDATAFL);
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

void getplanet(planettype **p, starnum_t star, planetnum_t pnum) {
  if (p >= &planets[0][0] && p < &planets[NUMSTARS][MAXPLANETS])
    ;    /* Do nothing */
  else { /* Allocate space for others */
    *p = (planettype *)malloc(sizeof(planettype));
  }
  int filepos = Stars[star]->planetpos[pnum];
  Fileread(pdata, (char *)*p, sizeof(planettype), filepos);
}

sector getsector(const planettype &p, const int x, const int y) {
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

sector_map getsmap(const planettype &p) {
  const char *tail;
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

int getship(shiptype **s, shipnum_t shipnum) {
  struct stat buffer;

  if (shipnum <= 0) return 0;

  fstat(shdata, &buffer);
  if (buffer.st_size / sizeof(shiptype) < shipnum)
    return 0;
  else {
    if ((*s = (shiptype *)malloc(sizeof(shiptype))) == NULL)
      printf("getship:malloc() error \n"), exit(0);

    Fileread(shdata, (char *)*s, sizeof(shiptype),
             (shipnum - 1) * sizeof(shiptype));
    return 1;
  }
}

int getcommod(commodtype **c, commodnum_t commodnum) {
  struct stat buffer;

  if (commodnum <= 0) return 0;

  fstat(commoddata, &buffer);
  if (buffer.st_size / sizeof(commodtype) < commodnum)
    return 0;
  else {
    if ((*c = (commodtype *)malloc(sizeof(commodtype))) == NULL)
      printf("getcommod:malloc() error \n"), exit(0);

    Fileread(commoddata, (char *)*c, sizeof(commodtype),
             (commodnum - 1) * sizeof(commodtype));
    return 1;
  }
}

/* gets the ship # listed in the top of the file SHIPFREEDATAFL. this
** might have no other uses besides build().
*/
int getdeadship(void) {
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
  } else
    close_file(fd);
  return -1;
}

int getdeadcommod(void) {
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
  } else
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
    const char *tail = 0;
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
    const char *tail = 0;
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
    const char *tail = 0;
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
    const char *tail = 0;
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
    const char *tail = 0;
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
  char *err_msg = 0;
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &err_msg);
}

static void end_bulk_insert() {
  char *err_msg = 0;
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &err_msg);
}

void putplanet(planettype *p, startype *star, int pnum) {
  start_bulk_insert();

  int filepos;
  filepos = star->planetpos[pnum];
  Filewrite(pdata, (char *)p, sizeof(planettype), filepos);
  const char *tail = 0;
  const char *plinfo_tail = 0;
  const char *plinfo_route_tail = 0;
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
  sqlite3_prepare_v2(db, plinfo_sql, -1, &plinfo_stmt, &plinfo_tail);

  const char *plinfo_route_sql =
      "REPLACE INTO tbl_plinfo_routes (planet_id, player_id, routenum, "
      "order_set, dest_star, dest_planet, "
      "load, unload, x, y) VALUES "
      "(?1, ?2, ?3, ?4, 5, ?6, ?7, ?8, ?9, ?10)";
  sqlite3_prepare_v2(db, plinfo_route_sql, -1, &plinfo_route_stmt,
                     &plinfo_route_tail);

  sqlite3_bind_int(stmt, 1, p->planet_id);
  sqlite3_bind_int(stmt, 2, star->star_id);
  sqlite3_bind_int(stmt, 3, pnum);
  sqlite3_bind_text(stmt, 4, star->pnames[pnum], strlen(star->pnames[pnum]),
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 5, p->xpos);
  sqlite3_bind_double(stmt, 6, p->ypos);
  sqlite3_bind_int(stmt, 7, p->ships);
  sqlite3_bind_int(stmt, 8, p->Maxx);
  sqlite3_bind_int(stmt, 9, p->Maxy);
  sqlite3_bind_int(stmt, 10, p->popn);
  sqlite3_bind_int(stmt, 11, p->troops);
  sqlite3_bind_int(stmt, 12, p->maxpopn);
  sqlite3_bind_int(stmt, 13, p->total_resources);
  sqlite3_bind_int(stmt, 14, p->slaved_to);
  sqlite3_bind_int(stmt, 15, p->type);
  sqlite3_bind_int(stmt, 16, p->expltimer);
  sqlite3_bind_int(stmt, 17, p->conditions[0]);
  sqlite3_bind_int(stmt, 18, p->conditions[1]);
  sqlite3_bind_int(stmt, 19, p->conditions[2]);
  sqlite3_bind_int(stmt, 20, p->conditions[3]);
  sqlite3_bind_int(stmt, 21, p->conditions[4]);
  sqlite3_bind_int(stmt, 22, p->conditions[5]);
  sqlite3_bind_int(stmt, 23, p->conditions[6]);
  sqlite3_bind_int(stmt, 24, p->conditions[7]);
  sqlite3_bind_int(stmt, 25, p->conditions[8]);
  sqlite3_bind_int(stmt, 26, p->conditions[9]);
  sqlite3_bind_int(stmt, 27, p->conditions[10]);
  sqlite3_bind_int(stmt, 28, p->explored);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
  }

  {
    for (player_t i = 0; i < MAXPLAYERS; i++) {
      sqlite3_bind_int(plinfo_stmt, 1, p->planet_id);
      sqlite3_bind_int(plinfo_stmt, 2, i);
      sqlite3_bind_int(plinfo_stmt, 3, p->info[i].fuel);
      sqlite3_bind_int(plinfo_stmt, 4, p->info[i].destruct);
      sqlite3_bind_int(plinfo_stmt, 5, p->info[i].resource);
      sqlite3_bind_int(plinfo_stmt, 6, p->info[i].popn);
      sqlite3_bind_int(plinfo_stmt, 7, p->info[i].troops);
      sqlite3_bind_int(plinfo_stmt, 8, p->info[i].crystals);
      sqlite3_bind_int(plinfo_stmt, 9, p->info[i].prod_res);
      sqlite3_bind_int(plinfo_stmt, 10, p->info[i].prod_fuel);
      sqlite3_bind_int(plinfo_stmt, 11, p->info[i].prod_dest);
      sqlite3_bind_int(plinfo_stmt, 12, p->info[i].prod_crystals);
      sqlite3_bind_int(plinfo_stmt, 13, p->info[i].prod_money);
      sqlite3_bind_double(plinfo_stmt, 14, p->info[i].prod_tech);
      sqlite3_bind_int(plinfo_stmt, 15, p->info[i].tech_invest);
      sqlite3_bind_int(plinfo_stmt, 16, p->info[i].numsectsowned);
      sqlite3_bind_int(plinfo_stmt, 17, p->info[i].comread);
      sqlite3_bind_int(plinfo_stmt, 18, p->info[i].mob_set);
      sqlite3_bind_int(plinfo_stmt, 19, p->info[i].tox_thresh);
      sqlite3_bind_int(plinfo_stmt, 20, p->info[i].explored);
      sqlite3_bind_int(plinfo_stmt, 21, p->info[i].autorep);
      sqlite3_bind_int(plinfo_stmt, 22, p->info[i].tax);
      sqlite3_bind_int(plinfo_stmt, 23, p->info[i].newtax);
      sqlite3_bind_int(plinfo_stmt, 24, p->info[i].guns);
      sqlite3_bind_int(plinfo_stmt, 25, p->info[i].mob_points);
      sqlite3_bind_double(plinfo_stmt, 26, p->info[i].est_production);

      if (sqlite3_step(plinfo_stmt) != SQLITE_DONE) {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
      }
      sqlite3_reset(plinfo_stmt);

      {
        for (int j = 0; j < MAX_ROUTES; j++) {
          sqlite3_bind_int(plinfo_route_stmt, 1, p->planet_id);
          sqlite3_bind_int(plinfo_route_stmt, 2, i);
          sqlite3_bind_int(plinfo_route_stmt, 3, j);
          sqlite3_bind_int(plinfo_route_stmt, 4, p->info[i].route[j].set);
          sqlite3_bind_int(plinfo_route_stmt, 5, p->info[i].route[j].dest_star);
          sqlite3_bind_int(plinfo_route_stmt, 6,
                           p->info[i].route[j].dest_planet);
          sqlite3_bind_int(plinfo_route_stmt, 7, p->info[i].route[j].load);
          sqlite3_bind_int(plinfo_route_stmt, 8, p->info[i].route[j].unload);
          sqlite3_bind_int(plinfo_route_stmt, 9, p->info[i].route[j].x);
          sqlite3_bind_int(plinfo_route_stmt, 10, p->info[i].route[j].y);

          if (sqlite3_step(plinfo_route_stmt) != SQLITE_DONE) {
            fprintf(stderr, "%s\n", sqlite3_errmsg(db));
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

void putsector(const sector &s, const planettype &p, const int x, const int y) {
  const char *tail = 0;
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
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
  }

  sqlite3_reset(stmt);
}

void putsmap(sector_map &map, planettype &p) {
  start_bulk_insert();

  for (int y = 0; y < p.Maxy; y++) {
    for (int x = 0; x < p.Maxx; x++) {
      auto &sec = map.get(x, y);
      putsector(sec, p, x, y);
    }
  }

  end_bulk_insert();
}

void putship(shiptype *s) {
  Filewrite(shdata, (char *)s, sizeof(shiptype),
            (s->number - 1) * sizeof(shiptype));
}

void putcommod(commodtype *c, int commodnum) {
  Filewrite(commoddata, (char *)c, sizeof(commodtype),
            (commodnum - 1) * sizeof(commodtype));
}

int Numraces(void) {
  struct stat buffer;

  fstat(racedata, &buffer);
  return ((int)(buffer.st_size / sizeof(racetype)));
}

shipnum_t Numships(void) /* return number of ships */
{
  struct stat buffer;

  fstat(shdata, &buffer);
  return ((int)(buffer.st_size / sizeof(shiptype)));
}

int Numcommods(void) {
  struct stat buffer;

  fstat(commoddata, &buffer);
  return ((int)(buffer.st_size / sizeof(commodtype)));
}

int Newslength(int type) {
  struct stat buffer;
  FILE *fp;

  switch (type) {
    case DECLARATION:
      if ((fp = fopen(DECLARATIONFL, "r")) == NULL)
        fp = fopen(DECLARATIONFL, "w+");
      break;

    case TRANSFER:
      if ((fp = fopen(TRANSFERFL, "r")) == NULL) fp = fopen(TRANSFERFL, "w+");
      break;
    case COMBAT:
      if ((fp = fopen(COMBATFL, "r")) == NULL) fp = fopen(COMBATFL, "w+");
      break;
    case ANNOUNCE:
      if ((fp = fopen(ANNOUNCEFL, "r")) == NULL) fp = fopen(ANNOUNCEFL, "w+");
      break;
    default:
      return 0;
  }
  fstat(fileno(fp), &buffer);
  fclose(fp);
  return ((int)buffer.st_size);
}

/* delete contents of dead ship file */
void clr_shipfree(void) { fclose(fopen(SHIPFREEDATAFL, "w+")); }

void clr_commodfree(void) { fclose(fopen(COMMODFREEDATAFL, "w+")); }

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
  } else {
    read(power_fd, (char *)p, sizeof(*p) * MAXPLAYERS);
    close_file(power_fd);
  }
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
  } else {
    read(block_fd, (char *)b, sizeof(*b) * MAXPLAYERS);
    close_file(block_fd);
  }
}
