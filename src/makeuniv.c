/* makeuniv.c -- universe creation program.
 *   Makes various required data files; calls makestar for each star desired.
 *
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 */

#include "GB_copyright.h"
#define EXTERN
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h" /* (for power) */
#include <math.h>
#include <string.h>

extern int Temperature(double, int);
extern void PrintStatistics(void);
extern void Makeplanet_init(void);
extern void Makestar_init(void);
extern startype *Makestar(FILE *, FILE *);
void InitFile(char *, void *, int);
void EmptyFile(char *);
void place_star(startype *);
void produce_postscript(char *);

#define DEFAULT_POSTSCRIPT_MAP_FILENAME "universe.ps"
#define NRINGS 3
#define RING_SPACING (10000 * (UNIVSIZE / 10000) / NRINGS)

int autoname_star = -1;
int autoname_plan = -1;
int minplanets = -1;
int maxplanets = -1;
int nstars = -1;
int planetlesschance = 0;
int printpostscript = 0;
int printplaninfo = 0;
int printstarinfo = 0;

static int occupied[100][100];

int main(int argc, char *argv[]) {
  FILE *stardata, *planetdata, *sectordata;
  char str[200];
  int c, i, star, x;
  double att, xspeed[NUMSTARS], yspeed[NUMSTARS];

  /*
   * Initialize: */
  srandom(getpid());
  Bzero(Sdata);

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
      case 'p':
        printpostscript = 1;
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
      default:
        printf("\n");
        printf("Unknown option \"%s\".\n", argv[i]);
      usage:
        printf("\n");
        printf("Usage: makeuniv [-a] [-b] [-e E] [-l MIN] [-m MAX] [-s N] [-v] "
               "[-w]\n");
        printf("  -a      Autoload star names.\n");
        printf("  -b      Autoload planet names.\n");
        printf("  -e E    Make E%% of stars have no planets.\n");
        printf("  -l MIN  Other systems will have at least MIN planets.\n");
        printf("  -m MAX  Other systems will have at most  MAX planets.\n");
        printf("  -p      Create postscript map file of the universe.\n");
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
    c = getchr();
    if (c != '\n')
      getchr();
    autoname_star = (c == 'y');
  }
  if (autoname_plan == -1) {
    printf("\nDo you wish to use the file \"%s\" for planet names? [y/n]> ",
           PLANETLIST);
    c = getchr();
    if (c != '\n')
      getchr();
    autoname_plan = (c == 'y');
  }
  while ((nstars < 1) || (nstars >= NUMSTARS)) {
    printf("Number of stars [1-%d]:", NUMSTARS - 1);
    scanf("%d", &nstars);
  }
  while ((minplanets <= 0) || (minplanets > MAXPLANETS)) {
    printf("Minimum number of planets per system [1-%d]: ", MAXPLANETS);
    scanf("%d", &minplanets);
  }
  while ((maxplanets < minplanets) || (maxplanets > MAXPLANETS)) {
    printf("Maximum number of planets per system [%d-%d]: ", minplanets,
           MAXPLANETS);
    scanf("%d", &maxplanets);
  }

  Makeplanet_init();
  Makestar_init();
  Sdata.numstars = nstars;
  sprintf(str, "/bin/mkdir %s", DATADIR);
  system(str);

  if (NULL == (planetdata = fopen(PLANETDATAFL, "w+"))) {
    printf("Unable to open planet data file \"%s\"\n", PLANETDATAFL);
    exit(-1);
  }
  if (NULL == (sectordata = fopen(SECTORDATAFL, "w+"))) {
    printf("Unable to open sector data file \"%s\"\n", SECTORDATAFL);
    exit(-1);
  }

  for (star = 0; star < nstars; star++) {
    Stars[star] = Makestar(planetdata, sectordata);
    xspeed[star] = 0;
    yspeed[star] = 0;
  }
  fchmod(planetdata, 00660); /* change data files to group readwrite */
  fclose(planetdata);
  fchmod(sectordata, 00660);
  fclose(sectordata);

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

  if ((stardata = fopen(STARDATAFL, "w+")) == NULL) {
    printf("Unable to open star data file \"%s\"\n", STARDATAFL);
    exit(-1);
  }
  fwrite(&(Sdata), sizeof(Sdata), 1, stardata);
  for (star = 0; star < Sdata.numstars; star++)
    fwrite(Stars[star], sizeof(startype), 1, stardata);
  fchmod(stardata, 00660);
  fclose(stardata);

  EmptyFile(SHIPDATAFL);
  EmptyFile(SHIPFREEDATAFL);
  EmptyFile(COMMODDATAFL);
  EmptyFile(COMMODFREEDATAFL);
  EmptyFile(PLAYERDATAFL);
  EmptyFile(RACEDATAFL);

  {
    struct power p[MAXPLAYERS];
    bzero((char *)p, sizeof(p));
    InitFile(POWFL, p, sizeof(p));
  }

  {
    struct block p[MAXPLAYERS];
    bzero((char *)p, sizeof(p));
    InitFile(BLOCKDATAFL, p, sizeof(p));
  }

  /*
   * Telegram files: directory and a file for each player. */
  sprintf(str, "/bin/mkdir %s", TELEGRAMDIR);
  system(str);
  chmod(TELEGRAMDIR, 00770);
#if 0  
  /* Why is this not needed any more? */
  for (i=1; i<MAXPLAYERS; i++) {
    sprintf(str, "%s.%d", TELEGRAMFL, i );
    EmptyFile(str) ;
    }
#endif

  /*
   * News files: directory and the 4 types of news. */
  sprintf(str, "/bin/mkdir %s", NEWSDIR);
  system(str);
  chmod(NEWSDIR, 00770);
  EmptyFile(DECLARATIONFL);
  EmptyFile(TRANSFERFL);
  EmptyFile(COMBATFL);
  EmptyFile(ANNOUNCEFL);

  PrintStatistics();

  if (printpostscript)
    produce_postscript(DEFAULT_POSTSCRIPT_MAP_FILENAME);
}

void InitFile(char *filename, void *ptr, int len) {
  FILE *f = fopen(filename, "w+");

  if (f == NULL) {
    printf("Unable to open \"%s\".\n", filename);
    exit(-1);
  }
  fwrite(ptr, len, 1, f);
  fchmod(f, 00660);
  fclose(f);
}

void EmptyFile(char *filename) { InitFile(filename, NULL, 0); }

/*
 * The procedure below was adapted from a program which is
 * Copyright: Andreas Girgensohn (andreasg@cs.colorado.edu)
 * produces a Postscript map of the universe. */
void produce_postscript(char *filename) {
  int min_x, max_x, min_y, max_y, i;
  double scale, nscale;
  FILE *f = fopen(filename, "w+");

  if (f == NULL) {
    printf("Unable to open postscript file \"%s\".\n", filename);
    return;
  }
  printf("Creating postscript file..");
  fflush(stdout);
  min_x = max_x = Stars[0]->xpos;
  min_y = max_y = Stars[0]->ypos;
  for (i = 1; i < nstars; i++) {
    if (Stars[i]->xpos < min_x)
      min_x = Stars[i]->xpos;
    if (Stars[i]->xpos > max_x)
      max_x = Stars[i]->xpos;
    if (Stars[i]->ypos < min_y)
      min_y = Stars[i]->ypos;
    if (Stars[i]->ypos > max_y)
      max_y = Stars[i]->ypos;
  }
  /* max map size: 8.5in x 11in sheet, 0.5in borders, */
  /* 0.5in on the right for star names */
  /* 72 points = 1in */
  scale = 7.0 * 72 / (max_x - min_x);
  nscale = 10.0 * 72 / (max_y - min_y);
  if (nscale < scale)
    scale = nscale;
  fprintf(f, "%%!PS-Adobe-2.0\n\n");
  /* 0,0 is in the topleft corner */
  fprintf(f, "0.5 72 mul 10.5 72 mul translate\n");
  fprintf(f, "/drawcircle\n");
  fprintf(f, "{\n");
  fprintf(f, "  newpath 0 360 arc stroke\n");
  fprintf(f, "}\n");
  fprintf(f, "def\n\n");
  fprintf(f, "/drawstar\n");
  fprintf(f, "{\n");
  fprintf(f, "  /starname exch def\n");
  fprintf(f, "  /ypos exch def\n");
  fprintf(f, "  /xpos exch def\n");
  fprintf(f, "  xpos ypos 2 drawcircle\n");
  fprintf(f, "  4 xpos add ypos moveto\n");
  fprintf(f, "  starname show\n");
  fprintf(f, "}\n");
  fprintf(f, "def\n\n");
  fprintf(f, "0 setlinewidth\n");
  fprintf(f, "newpath -10 10 moveto 7.5 72 mul 10 add 10 lineto\n");
  fprintf(f, "7.5 72 mul 10 add %d lineto -10 %d lineto closepath clip\n",
          (int)((min_y - max_y) * scale) - 10,
          (int)((min_y - max_y) * scale) - 10);
#if 0
  /*
   * Print scale rings from center of universe (0,0): */
  fprintf ("\n/Times-Bold findfont 9 scalefont setfont\n\n");
  for (i = 1; i <= NRINGS; i++)
    fprintf(f, "%d %d %d drawcircle\n",
	    (int) ( - min_x * scale), (int) (min_y * scale),
	    (int) (i * RING_SPACING * scale)) ;
#endif
  /*
   * Print each star. */
  fprintf(f, "\n/Times-Roman findfont 8 scalefont setfont\n\n");
  for (i = 0; i < nstars; i++)
    fprintf(f, "%d %d (%s) drawstar\n", (int)((Stars[i]->xpos - min_x) * scale),
            (int)((min_y - Stars[i]->ypos) * scale), Stars[i]->name);
  fprintf(f, "\nshowpage\n");
  fclose(f);
  printf("done\n");
}

void place_star(startype *star) {
  int found = 0, i, j;
  while (!found) {
    star->xpos = (double)int_rand(-UNIVSIZE, UNIVSIZE);
    star->ypos = (double)int_rand(-UNIVSIZE, UNIVSIZE);
    /* check to see if another star is nearby */
    i = 100 * ((int)star->xpos + UNIVSIZE) / (2 * UNIVSIZE);
    j = 100 * ((int)star->xpos + UNIVSIZE) / (2 * UNIVSIZE);
    if (!occupied[i][j])
      occupied[i][j] = found = 1;
  }
  return;
}
