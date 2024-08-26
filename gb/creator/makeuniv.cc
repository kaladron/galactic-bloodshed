// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* makeuniv.c -- universe creation program.
 *   Makes various required data files; calls makestar for each star desired. */

import gblib;
import std.compat;

#include "gb/creator/makeuniv.h"

#include <sqlite3.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "gb/creator/makestar.h"
#include "gb/files.h"
#include "gb/globals.h"

int autoname_star = -1;
int autoname_plan = -1;
int minplanets = -1;
int maxplanets = -1;
int printplaninfo = 0;
int printstarinfo = 0;

static int nstars = -1;
static int occupied[100][100];
static int planetlesschance = 0;

namespace {
void InitFile(const std::string &path, void *buffer = nullptr, size_t len = 0) {
  const char *filename = path.c_str();
  FILE *f = fopen(filename, "w+");
  if (buffer != nullptr && len > 0) {
    if (f == nullptr) {
      printf("Unable to open \"%s\".\n", filename);
      exit(-1);
    }
    fwrite(buffer, len, 1, f);
  }
  chmod(filename, 00660);
  fclose(f);
}

void EmptyFile(const std::string &path) { InitFile(path); }
};  // namespace

int main(int argc, char *argv[]) {
  int c;
  int i;

  /*
   * Initialize: */
  srandom(getpid());
  bzero(&Sdata, sizeof(Sdata));

  /*
   * Read the arguments for values: */
  for (i = 1; i < argc; i++)
    if (argv[i][0] != '-')
      goto usage;
    else
      switch (argv[i][1]) {
        case 'a':
          autoname_star = 1;
          break;
        case 'b':
          autoname_plan = 1;
          break;
        case 'e':
          planetlesschance = atoi(argv[++i]);
          break;
        case 'l':
          minplanets = atoi(argv[++i]);
          break;
        case 'm':
          maxplanets = atoi(argv[++i]);
          break;
        case 's':
          nstars = atoi(argv[++i]);
          break;
        case 'v':
          printplaninfo = 1;
          break;
        case 'w':
          printstarinfo = 1;
          break;
        case 'd':
          autoname_star = 1;
          autoname_plan = 1;
          printplaninfo = 1;
          printstarinfo = 1;
          nstars = 128;
          minplanets = 1;
          maxplanets = 10;
          break;
        default:
          printf("\n");
          printf("Unknown option \"%s\".\n", argv[i]);
        usage:
          printf("\n");
          printf(
              "Usage: makeuniv [-a] [-b] [-e E] [-l MIN] [-m MAX] [-s N] [-v] "
              "[-w]\n");
          printf("  -a      Autoload star names.\n");
          printf("  -b      Autoload planet names.\n");
          printf("  -d      Use all defauls and autoloaded names.\n");
          printf("  -e E    Make E%% of stars have no planets.\n");
          printf("  -l MIN  Other systems will have at least MIN planets.\n");
          printf("  -m MAX  Other systems will have at most  MAX planets.\n");
          printf("  -s S    The universe will have S stars.\n");
          printf("  -v      Print info and map of planets generated.\n");
          printf("  -w      Print info on stars generated.\n");
          printf("\n");
          exit(0);
      }

  /*
   * Get values for all the switches that still don't have good values. */
  if (autoname_star == -1) {
    printf("\nDo you wish to use the file \"%s\" for star names? [y/n]> ",
           STARLIST);
    c = std::getchar();
    if (c != '\n') std::getchar();
    autoname_star = (c == 'y');
  }
  if (autoname_plan == -1) {
    printf("\nDo you wish to use the file \"%s\" for planet names? [y/n]> ",
           PLANETLIST);
    c = std::getchar();
    if (c != '\n') std::getchar();
    autoname_plan = (c == 'y');
  }
  while ((nstars < 1) || (nstars >= NUMSTARS)) {
    printf("Number of stars [1-%d]:", NUMSTARS - 1);
    if (scanf("%d", &nstars) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
  }
  while ((minplanets <= 0) || (minplanets > MAXPLANETS)) {
    printf("Minimum number of planets per system [1-%d]: ", MAXPLANETS);
    if (scanf("%d", &minplanets) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
  }
  while ((maxplanets < minplanets) || (maxplanets > MAXPLANETS)) {
    printf("Maximum number of planets per system [%d-%d]: ", minplanets,
           MAXPLANETS);
    if (scanf("%d", &maxplanets) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
  }

  Makeplanet_init();
  Makestar_init();
  Sdata.numstars = nstars;

  Sql db{};
  initsqldata();

  for (starnum_t star = 0; star < nstars; star++) {
    stars.emplace_back(Makestar(star));
  }

#if 0
  /* 
   * Try to more evenly space stars.  Essentially this is an inverse-gravity
   * calculation: the nearer two stars are to each other, the more they
   * repulse each other.  Several iterations of this will suffice to move all
   * of the stars nicely apart. */
  for (i=0; i<CREAT_UNIV_ITERAT; i++)
    for (star=0; star<Sdata.numstars; star++) {
      for (x=0; x<Sdata.numstars; x++)	/* star2 */
	if (x!=star) {
	  /* find inverse of distance squared */
	  att = 10*UNIVSIZE / Distsq(Stars[star]->xpos, Stars[star]->ypos, Stars[x]->xpos, Stars[x]->ypos);
	  xspeed[star] += att * (Stars[star]->xpos - Stars[x]->xpos);
	  if (Stars[star]->xpos>UNIVSIZE || Stars[star]->xpos< -UNIVSIZE)
	    xspeed[star] *= -1;
	  yspeed[star] += att * (Stars[star]->ypos - Stars[x]->ypos);
	  if (Stars[star]->ypos>UNIVSIZE || Stars[star]->ypos< -UNIVSIZE)
	    yspeed[star] *= -1;
	  }
      Stars[star]->xpos += xspeed[star];
      Stars[star]->ypos += yspeed[star];
      }
#endif

  db.putsdata(&Sdata);
  for (starnum_t star = 0; star < Sdata.numstars; star++)
    db.putstar(stars[star], star);
  chmod(STARDATAFL, 00660);

  EmptyFile(SHIPDATAFL);
  EmptyFile(SHIPFREEDATAFL);
  EmptyFile(COMMODDATAFL);
  EmptyFile(COMMODFREEDATAFL);
  EmptyFile(PLAYERDATAFL);
  EmptyFile(RACEDATAFL);

  {
    struct power p[MAXPLAYERS];
    bzero((char *)p, sizeof(p));
    putpower(p);
  }

  {
    struct block p[MAXPLAYERS];
    bzero((char *)p, sizeof(p));
    InitFile(BLOCKDATAFL, p, sizeof(p));
  }

  /*
   * Telegram files: directory and a file for each player. */
  mkdir(TELEGRAMDIR, 00770);
#if 0  
  /* Why is this not needed any more? */
  for (i=1; i<MAXPLAYERS; i++) {
    sprintf(str, "%s.%d", TELEGRAMFL, i );
    EmptyFile(str) ;
    }
#endif

  /*
   * News files: directory and the 4 types of news. */
  mkdir(NEWSDIR, 00770);
  EmptyFile(DECLARATIONFL);
  EmptyFile(TRANSFERFL);
  EmptyFile(COMBATFL);
  EmptyFile(ANNOUNCEFL);

  PrintStatistics();

  return 0;
}

void place_star(Star &star) {
  int found = 0;
  int i;
  int j;
  while (!found) {
    star.xpos = (double)int_rand(-UNIVSIZE, UNIVSIZE);
    star.ypos = (double)int_rand(-UNIVSIZE, UNIVSIZE);
    /* check to see if another star is nearby */
    i = 100 * ((int)star.xpos + UNIVSIZE) / (2 * UNIVSIZE);
    j = 100 * ((int)star.xpos + UNIVSIZE) / (2 * UNIVSIZE);
    if (!occupied[i][j]) occupied[i][j] = found = 1;
  }
}
