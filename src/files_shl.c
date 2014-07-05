/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 *
 */
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "GB_copyright.h"
#define EXTERN extern
#define SHIP_CONSISTENCY
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

int commoddata, pdata, racedata, sectdata, shdata, stdata;

extern int errno;
int sys_nerr;

void close_file(int);
void open_data_files(void);
void close_data_files(void);
void openstardata(int *);
void openshdata(int *);
void opencommoddata(int *);
void openpdata(int *);
void opensectdata(int *);
void openracedata(int *);
void getsdata(struct stardata *S);
#ifdef DEBUG
void DEBUGgetrace(racetype **, int, char *, int);
void DEBUGgetstar(startype **, int, char *, int);
void DEBUGgetplanet(planettype **, int, int, char *, int);
int DEBUGgetship(shiptype **, int, char *, int);
int DEBUGgetcommod(commodtype **, int, char *, int);
#else
void getrace(racetype **, int);
void getstar(startype **, int);
void getplanet(planettype **, int, int);
int getship(shiptype **, int);
int getcommod(commodtype **, int);
#endif
void getsector(sectortype **, planettype *, int, int);
void getsmap(sectortype *, planettype *);
int getdeadship(void);
int getdeadcommod(void);
void putsdata(struct stardata *);
void putrace(racetype *);
void putstar(startype *, int);
void putplanet(planettype *, int, int);
void putsector(sectortype *, planettype *, int, int);
void putsmap(sectortype *, planettype *);
void putship(shiptype *);
void putcommod(commodtype *, int);
int Numraces(void);
int Numships(void);
int Numcommods(void);
int Newslength(int);
void clr_shipfree(void);
void clr_commodfree(void);
void makeshipdead(int);
void makecommoddead(int);
void Putpower(struct power[MAXPLAYERS]);
void Getpower(struct power[MAXPLAYERS]);
void Putblock(struct block[MAXPLAYERS]);
void Getblock(struct block[MAXPLAYERS]);
#include "files_rw.p"

void close_file(int fd) { close(fd); }

void open_data_files(void) {
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

#ifdef DEBUG
void DEBUGgetrace(racetype **r, int rnum, char *fname, int lineno)
#else
void getrace(racetype **r, int rnum)
#endif
{
#ifdef DEBUG
  *r = (racetype *)DEBUGmalloc(sizeof(racetype), fname, lineno);
#else
  *r = (racetype *)malloc(sizeof(racetype));
#endif
  Fileread(racedata, (char *)*r, sizeof(racetype),
           (rnum - 1) * sizeof(racetype));
}

#ifdef DEBUG
void DEBUGgetstar(startype **s, int star, char *fname, int lineno)
#else
void getstar(startype **s, int star)
#endif
{
  if (s >= &Stars[0] && s < &Stars[NUMSTARS])
    ; /* Do nothing */
  else {
#ifdef DEBUG
    *s = (startype *)DEBUGmalloc(sizeof(startype), fname, lineno);
#else
    *s = (startype *)malloc(sizeof(startype));
#endif
  }
  Fileread(stdata, (char *)*s, sizeof(startype),
           (int)(sizeof(Sdata) + star * sizeof(startype)));
}

#ifdef DEBUG
void DEBUGgetplanet(planettype **p, int star, int pnum, char *fname, int lineno)
#else
void getplanet(planettype **p, int star, int pnum)
#endif
{
  int filepos;
  if (p >= &planets[0][0] && p < &planets[NUMSTARS][MAXPLANETS])
    ;    /* Do nothing */
  else { /* Allocate space for others */
#ifdef DEBUG
    *p = (planettype *)DEBUGmalloc(sizeof(planettype), fname, lineno);
#else
    *p = (planettype *)malloc(sizeof(planettype));
#endif
  }
  filepos = Stars[star]->planetpos[pnum];
  Fileread(pdata, (char *)*p, sizeof(planettype), filepos);
}

void getsector(sectortype **s, planettype *p, int x, int y) {
  int filepos;
  filepos = p->sectormappos + (y * p->Maxx + x) * sizeof(sectortype);
  *s = (sectortype *)malloc(sizeof(sectortype));
  Fileread(sectdata, (char *)*s, sizeof(sectortype), filepos);
}

void getsmap(sectortype *map, planettype *p) {
  Fileread(sectdata, (char *)map, p->Maxx * p->Maxy * sizeof(sectortype),
           p->sectormappos);
}

#ifdef DEBUG
int DEBUGgetship(shiptype **s, int shipnum, char *fname, int lineno)
#else
int getship(shiptype **s, int shipnum)
#endif
{
  struct stat buffer;

  if (shipnum <= 0)
    return 0;

  fstat(shdata, &buffer);
  if (buffer.st_size / sizeof(shiptype) < shipnum)
    return 0;
  else {

#ifdef DEBUG
    if ((*s = (shiptype *)DEBUGmalloc(sizeof(shiptype), fname, lineno)) == NULL)
#else
    if ((*s = (shiptype *)malloc(sizeof(shiptype))) == NULL)
#endif
      printf("getship:Malloc() error \n"), exit(0);

    Fileread(shdata, (char *)*s, sizeof(shiptype),
             (shipnum - 1) * sizeof(shiptype));
    return 1;
  }
}

#ifdef DEBUG
int DEBUGgetcommod(commodtype **c, int commodnum, char *fname, int lineno)
#else
int getcommod(commodtype **c, int commodnum)
#endif
{
  struct stat buffer;

  if (commodnum <= 0)
    return 0;

  fstat(commoddata, &buffer);
  if (buffer.st_size / sizeof(commodtype) < commodnum)
    return 0;
  else {

#ifdef DEBUG
    if ((*c = (commodtype *)DEBUGmalloc(sizeof(commodtype), fname, lineno)) ==
        NULL)
#else
    if ((*c = (commodtype *)malloc(sizeof(commodtype))) == NULL)
#endif
      printf("getcommod:Malloc() error \n"), exit(0);

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

void putstar(startype *s, int snum) {
  Filewrite(stdata, (char *)s, sizeof(startype),
            (int)(sizeof(Sdata) + snum * sizeof(startype)));
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

int Numships(void) /* return number of ships */
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
    if ((fp = fopen(TRANSFERFL, "r")) == NULL)
      fp = fopen(TRANSFERFL, "w+");
    break;
  case COMBAT:
    if ((fp = fopen(COMBATFL, "r")) == NULL)
      fp = fopen(COMBATFL, "w+");
    break;
  case ANNOUNCE:
    if ((fp = fopen(ANNOUNCEFL, "r")) == NULL)
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

  if (shipno == 0)
    return;

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

  if (commodno == 0)
    return;

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
