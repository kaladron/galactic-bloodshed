// SPDX-License-Identifier: Apache-2.0

// \file makeuniv.cc Universe creation program.

import dallib;
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

int autoname_star = -1;
int autoname_plan = -1;
int minplanets = -1;
int maxplanets = -1;
int printplaninfo = 0;
int printstarinfo = 0;

static int nstars = -1;
static int occupied[100][100];
static int planetlesschance = 0;

// Local storage for universe creation - not the global Sdata/stars
static universe_struct Sdata{};
static std::vector<Star> stars;

int main(int argc, char* argv[]) {
  int c;
  int i;

  /*
   * Initialize: */
  srandom(getpid());

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
          std::println("");
          std::println("Unknown option \"{}\".", argv[i]);
usage:
          std::println("");
          std::println(
              "Usage: makeuniv [-a] [-b] [-e E] [-l MIN] [-m MAX] [-s N] [-v] "
              "[-w]");
          std::println("  -a      Autoload star names.");
          std::println("  -b      Autoload planet names.");
          std::println("  -d      Use all defauls and autoloaded names.");
          std::println("  -e E    Make E% of stars have no planets.");
          std::println(
              "  -l MIN  Other systems will have at least MIN planets.");
          std::println(
              "  -m MAX  Other systems will have at most  MAX planets.");
          std::println("  -s S    The universe will have S stars.");
          std::println("  -v      Print info and map of planets generated.");
          std::println("  -w      Print info on stars generated.");
          std::println("");
          exit(0);
      }

  /*
   * Get values for all the switches that still don't have good values. */
  if (autoname_star == -1) {
    std::println("\nDo you wish to use the file \"{}\" for star names? [y/n]> ",
                 STARLIST);
    c = std::getchar();
    if (c != '\n') std::getchar();
    autoname_star = (c == 'y');
  }
  if (autoname_plan == -1) {
    std::println(
        "\nDo you wish to use the file \"{}\" for planet names? [y/n]> ",
        PLANETLIST);
    c = std::getchar();
    if (c != '\n') std::getchar();
    autoname_plan = (c == 'y');
  }
  while ((nstars < 1) || (nstars >= NUMSTARS)) {
    std::println("Number of stars [1-{}]:", NUMSTARS - 1);
    if (scanf("%d", &nstars) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
  }
  while ((minplanets <= 0) || (minplanets > MAXPLANETS)) {
    std::println("Minimum number of planets per system [1-{}]: ", MAXPLANETS);
    if (scanf("%d", &minplanets) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
  }
  while ((maxplanets < minplanets) || (maxplanets > MAXPLANETS)) {
    std::println("Maximum number of planets per system [{}-{}]: ", minplanets,
                 MAXPLANETS);
    if (scanf("%d", &maxplanets) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
  }

  Makeplanet_init();
  Makestar_init();
  Sdata.numstars = nstars;

  // Create database and initialize schema
  Database db(PKGSTATEDIR "gb.db");
  initialize_schema(db);

  for (starnum_t star = 0; star < nstars; star++) {
    stars.emplace_back(Makestar(db, star));
  }

  // Count non-asteroid planets for victory conditions
  Sdata.planet_count =
      static_cast<planetnum_t>(db.count_non_asteroid_planets());

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

  // Save universe data and all stars to database using repositories
  JsonStore store(db);
  UniverseRepository universe_repo(store);
  Sdata.id = 1;  // Universe data is a singleton with id=1
  universe_repo.save(Sdata);

  StarRepository star_repo(store);
  for (starnum_t star = 0; star < Sdata.numstars; star++) {
    star_repo.save(stars[star]);
  }

  // Initialize power and block array for all players
  {
    JsonStore store(db);
    BlockRepository block_repo(store);
    PowerRepository power_repo(store);

    for (int i : std::views::iota(0, MAXPLAYERS)) {
      power p{};
      p.id = i;
      power_repo.save(p);

      block b{};
      b.Playernum = i;
      block_repo.save(b);
    }
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

  PrintStatistics();

  return 0;
}

void place_star(star_struct& star) {
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
