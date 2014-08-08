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
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "files.h"
#include "files_rw.h"
#include "power.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

static int commoddata, pdata, racedata, sectdata, shdata, stdata;

static sqlite3 *db;

void close_file(int fd) { close(fd); }

void initsqldata() __attribute__((no_sanitize_memory)) {
  const char *tbl_create = R"(
      CREATE TABLE tbl_planet(
          planet_id INT PRIMARY KEY NOT NULL, star_id INT NOT NULL,
          name TEXT NOT NULL, planet_order INT NOT NULL, xpos DOUBLE,
          ypos DOUBLE, ships INT64, Maxx INT, Maxy INT, popn INT64,
          troops INT64, maxpopn INT64, total_resources INT64, slaved_to INT,
          type INT, expltimer INT, condition_rtemp INT, condition_temp INT,
          condition_methane INT, condition_oxygen INT, condition_co2 INT,
          condition_hydrogen INT, condition_nitrogen INT, condition_sulfur INT,
          condition_helium INT, condition_other INT, condition_toxic INT,
          explored INT);

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
  opensectdata(&sectdata);
  openshdata(&shdata);
  openstardata(&stdata);
}

void close_data_files(void) {
  close_file(commoddata);
  close_file(pdata);
  close_file(racedata);
  close_file(sectdata);
  close_file(shdata);
  close_file(stdata);
}

void openstardata(int *fd) {
  /*printf(" openstardata\n");*/
  if ((*fd = open(STARDATAFL, O_RDWR, 0777)) < 0) {
    perror("openstardata");
    printf("unable to open %s\n", STARDATAFL);
    exit(-1);
  }
}

void openshdata(int *fd) {
  if ((*fd = open(SHIPDATAFL, O_RDWR, 0777)) < 0) {
    perror("openshdata");
    printf("unable to open %s\n", SHIPDATAFL);
    exit(-1);
  }
}

void opencommoddata(int *fd) {
  if ((*fd = open(COMMODDATAFL, O_RDWR, 0777)) < 0) {
    perror("opencommoddata");
    printf("unable to open %s\n", COMMODDATAFL);
    exit(-1);
  }
}

void openpdata(int *fd) {
  if ((*fd = open(PLANETDATAFL, O_RDWR, 0777)) < 0) {
    perror("openpdata");
    printf("unable to open %s\n", PLANETDATAFL);
    exit(-1);
  }
}

void opensectdata(int *fd) {
  if ((*fd = open(SECTORDATAFL, O_RDWR, 0777)) < 0) {
    perror("opensectdata");
    printf("unable to open %s\n", SECTORDATAFL);
    exit(-1);
  }
}

void openracedata(int *fd) {
  if ((*fd = open(RACEDATAFL, O_RDWR, 0777)) < 0) {
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
  Fileread(stdata, (char *)*s, sizeof(startype),
           (int)(sizeof(Sdata) + star * sizeof(startype)));
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

void getsector(sectortype **s, planettype *p, int x, int y) {
  int filepos;
  filepos = p->sectormappos + (y * p->Maxx + x) * sizeof(sectortype);
  *s = (sectortype *)malloc(sizeof(sectortype));
  Fileread(sectdata, (char *)*s, sizeof(sectortype), filepos);
}

void getsmap(const sectortype *map, const planettype *p) {
  Fileread(sectdata, (char *)map, p->Maxx * p->Maxy * sizeof(sectortype),
           p->sectormappos);
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

int getcommod(commodtype **c, int commodnum) {
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

  char *err_msg = 0;
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &err_msg);

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

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
  }

  {
    const char *tail = 0;
    sqlite3_stmt *stmt;
    const char *sql =
        "REPLACE INTO tbl_star_governor (star_id, player_id, governor_id) "
        "VALUES (?1, ?2, ?3)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    for (player_t i = 0; i < MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, s->governor[i]);

      sqlite3_step(stmt);

      sqlite3_clear_bindings(stmt);
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
    for (player_t i = 0; i < MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, s->AP[i]);

      sqlite3_step(stmt);

      sqlite3_clear_bindings(stmt);
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
    for (player_t i = 0; i < MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, isset(s->explored, i) ? 1 : 0);

      sqlite3_step(stmt);

      sqlite3_clear_bindings(stmt);
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
    for (player_t i = 0; i < MAXPLAYERS; i++) {
      sqlite3_bind_int(stmt, 1, snum);
      sqlite3_bind_int(stmt, 2, i);
      sqlite3_bind_int(stmt, 3, isset(s->inhabited, i) ? 1 : 0);

      sqlite3_step(stmt);

      sqlite3_clear_bindings(stmt);
      sqlite3_reset(stmt);
    }
  }

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &err_msg);
}

void putplanet(planettype *p, int star, int pnum) {
  int filepos;
  filepos = Stars[star]->planetpos[pnum];
  Filewrite(pdata, (char *)p, sizeof(planettype), filepos);
}

void putsector(sectortype *s, planettype *p, int x, int y) {
  int filepos;
  filepos = p->sectormappos + (y * p->Maxx + x) * sizeof(sectortype);
  Filewrite(sectdata, (char *)s, sizeof(sectortype), filepos);
}

void putsmap(sectortype *map, planettype *p) {
  Filewrite(sectdata, (char *)map, p->Maxx * p->Maxy * sizeof(sectortype),
            p->sectormappos);
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
