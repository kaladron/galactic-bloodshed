/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 * enrol.c -- initializes to owner one sector and planet.
 */

#include "GB_copyright.h"

#define EXTERN
#include "vars.h"
#include "ships.h"
#include "shipdata.h"
#include "races.h"
#include "buffers.h"

extern int errno;
racetype *Race;

int planet_translate[N_HOME_PLANET_TYPES] = { 0, 6, 7, 5, 2, 3, 4 };

#define DEFAULT_ENROLLMENT_FILENAME "enroll.saves"
#define DEFAULT_ENROLLMENT_FAILURE_FILENAME "failures.saves"

#ifdef __STDC__
void srandom(int seed);
long random(void);

int getpid(void);

void modify_print_loop(int level);

#else
#define const
#endif

void notify(who, gov, msg) int who, gov;
char *msg;
{ /* this is a dummy routine */
}

void warn(who, gov, msg) int who, gov;
char *msg;
{ /* this is a dummy routine */
}

void push_message(what, who, msg) int what, who;
char *msg;
{ /* this is a dummy routine */
}

void enroll_init() { srandom(getpid()); }

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race() {
  int mask, x, y, star, pnum, i, ppref, Playernum;
  int last_star_left, indirect[NUMSTARS];
  sectortype *sect;
  planettype *planet;
  startype *star_arena;

  if (race.status == STATUS_ENROLLED) {
    sprintf(race.rejection, "This race has already been enrolled!");
    return 1;
  }
  open_data_files();
  Playernum = Numraces() + 1;
  if ((Playernum == 1) && (race.priv_type != P_GOD)) {
    close_data_files();
    sprintf(race.rejection,
            "The first race enrolled must have God privileges.\n");
    return 1;
  }
  if (Playernum >= MAXPLAYERS) {
    close_data_files();
    sprintf(race.rejection, "There are already %d players; No more allowed.\n",
            MAXPLAYERS - 1);
    race.status = STATUS_UNENROLLABLE;
    return 1;
  }

  getsdata(&Sdata);
  star_arena = (startype *)malloc(Sdata.numstars * sizeof(startype));
  for (star = 0; star < Sdata.numstars; star++) {
    Stars[star] = &star_arena[star];
    getstar(&(Stars[star]), star);
  }

  printf("Looking for %s..", planet_print_name[race.home_planet_type]);
  fflush(stdout);

  ppref = planet_translate[race.home_planet_type];
  for (i = 0; i < Sdata.numstars; i++)
    indirect[i] = i;
  last_star_left = Sdata.numstars - 1;
  while (last_star_left >= 0) {
    i = int_rand(0, last_star_left);
    star = indirect[i];

    printf(".", star);
    fflush(stdout);
    /*
     * Skip over inhabited stars and stars with few planets. */
    if ((Stars[star]->numplanets < 2) || Stars[star]->inhabited[0] ||
        Stars[star]->inhabited[1]) {
    } else {
      /* look for uninhabited planets */
      for (pnum = 0; pnum < Stars[star]->numplanets; pnum++) {
        getplanet(&planet, star, pnum);
        if ((planet->type == ppref) && (planet->conditions[RTEMP] >= -200) &&
            (planet->conditions[RTEMP] <= 100))
          goto found_planet;
        free(planet);
      }
    }
    /*
     * Since we are here, this star didn't work out: */
    indirect[i] = indirect[last_star_left--];
  }

  /*
   * If we get here, then we did not find any good planet. */
  printf(" failed!\n");
  sprintf(race.rejection,
          "Didn't find any free %s; choose another home planet type.\n",
          planet_print_name[race.home_planet_type]);
  close_data_files();
  race.status = STATUS_UNENROLLABLE;
  return 1;

found_planet:
  printf(" found!\n");
  Race = Malloc(racetype);
  Bzero(*Race);

  Race->Playernum = Playernum;
  Race->God = (race.priv_type == P_GOD);
  Race->Guest = (race.priv_type == P_GUEST);
  strcpy(Race->name, race.name);
  strcpy(Race->password, race.password);

  strcpy(Race->governor[0].password, "0");
  Race->governor[0].homelevel = Race->governor[0].deflevel = LEVEL_PLAN;
  Race->governor[0].homesystem = Race->governor[0].defsystem = star;
  Race->governor[0].homeplanetnum = Race->governor[0].defplanetnum = pnum;
  /* display options */
  Race->governor[0].toggle.highlight = Playernum;
  Race->governor[0].toggle.inverse = 1;
  Race->governor[0].toggle.color = 0;
  Race->governor[0].active = 1;

  for (i = 0; i <= OTHER; i++)
    Race->conditions[i] = planet->conditions[i];
#if 0
  /* make conditions preferred by your people set to (more or less) 
     those of the planet : higher the concentration of gas, the higher
     percentage difference between planet and race */
  for (j=0; j<=OTHER; j++)
    Race->conditions[j] = planet->conditions[j]
      + int_rand(round_rand(-planet->conditions[j]*2.0), 
		 round_rand(planet->conditions[j]*2.0) ) ;
#endif

  for (i = 0; i < MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == Playernum) || (Playernum == 1) || Race->God)
      Race->translate[i - 1] = 100; /* you can talk to own race */
    else
      Race->translate[i - 1] = 1;
  }

#if 0
  /* All of the following zeros are not really needed, because the race
     was bzero'ed out above. */
  for (i=0; i<80; i++)
    Race->discoveries[i] = 0;
  Race->tech = 0.0;
  Race->morale = 0;
  Race->turn = 0;
  Race->allied[0] = Race->allied[1] = 0;
  Race->atwar[0] = Race->atwar[1] = 0;
  for (i=0; i<MAXPLAYERS; i++) 
    Race->points[i]=0;
#endif

  /*
   * Assign racial characteristics. */
  Race->absorb = race.attr[ABSORB];
  Race->collective_iq = race.attr[COL_IQ];
  Race->Metamorph = (race.race_type == R_METAMORPH);
  Race->pods = race.attr[PODS];

  Race->fighters = race.attr[FIGHT];
  if (race.attr[COL_IQ] == 1.0)
    Race->IQ_limit = race.attr[A_IQ];
  else
    Race->IQ = race.attr[A_IQ];
  Race->number_sexes = race.attr[SEXES];

  Race->fertilize = race.attr[FERT] * 100;

  Race->adventurism = race.attr[ADVENT];
  Race->birthrate = race.attr[BIRTH];
  Race->mass = race.attr[MASS];
  Race->metabolism = race.attr[METAB];

  /*
   * Assign sector compats and determine a primary sector type. */
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++) {
    Race->likes[i] = race.compat[i] / 100.0;
    if ((100 == race.compat[i]) &&
        (1.0 == planet_compat_cov[race.home_planet_type][i]))
      Race->likesbest = i;
  }

  /*
   * Find sector to build capital on, and populate it: */
  getsmap(Smap, planet);
  PermuteSects(planet);
  Getxysect(planet, 0, 0, 1);
  while (i = Getxysect(planet, &x, &y, 0))
    if (Sector(*planet, x, y).des == Race->likesbest)
      break;
  if (!i)
    x = y = 0;
  sect = &Sector(*planet, x, y);
  sect->des = Race->likesbest;
  sect->owner = Playernum;
  sect->popn = planet->popn = Race->number_sexes;
  sect->fert = 100;
  sect->eff = 10;
  sect->troops = planet->troops = 0;

  Race->governors = 0;

  mask = sigblock(SIGBLOCKS);
  /* build a capital ship to run the government */
  {
    shiptype s;
    int shipno;

    Bzero(s);
    shipno = Numships() + 1;
    Race->Gov_ship = shipno;
    planet->ships = shipno;
    s.nextship = 0;

    s.type = OTYPE_GOV;
    s.xpos = Stars[star]->xpos + planet->xpos;
    s.ypos = Stars[star]->ypos + planet->ypos;
    s.land_x = x;
    s.land_y = y;

    s.speed = 0;
    s.owner = Playernum;
    s.governor = 0;

    s.tech = 100.0;

    s.build_type = OTYPE_GOV;
    s.armor = Shipdata[OTYPE_GOV][ABIL_ARMOR];
    s.guns = PRIMARY;
    s.primary = Shipdata[OTYPE_GOV][ABIL_GUNS];
    s.primtype = Shipdata[OTYPE_GOV][ABIL_PRIMARY];
    s.secondary = Shipdata[OTYPE_GOV][ABIL_GUNS];
    s.sectype = Shipdata[OTYPE_GOV][ABIL_SECONDARY];
    s.max_crew = Shipdata[OTYPE_GOV][ABIL_MAXCREW];
    s.max_destruct = Shipdata[OTYPE_GOV][ABIL_DESTCAP];
    s.max_resource = Shipdata[OTYPE_GOV][ABIL_CARGO];
    s.max_fuel = Shipdata[OTYPE_GOV][ABIL_FUELCAP];
    s.max_speed = Shipdata[OTYPE_GOV][ABIL_SPEED];
    s.build_cost = Shipdata[OTYPE_GOV][ABIL_COST];
    s.size = 100;
    s.base_mass = 100.0;
    sprintf(s.class, "Standard");

    s.fuel = 0.0;
    s.popn = Shipdata[s.type][ABIL_MAXCREW];
    s.troops = 0;
    s.mass = s.base_mass + Shipdata[s.type][ABIL_MAXCREW] * Race->mass;
    s.destruct = s.resource = 0;

    s.alive = 1;
    s.active = 1;
    s.protect.self = 1;

    s.docked = 1;
    /* docked on the planet */
    s.whatorbits = LEVEL_PLAN;
    s.whatdest = LEVEL_PLAN;
    s.deststar = star;
    s.destpnum = pnum;
    s.storbits = star;
    s.pnumorbits = pnum;
    s.rad = 0;
    s.damage = 0; /*Shipdata[s.type][ABIL_DAMAGE];*/
    /* (first capital is 100% efficient */
    s.retaliate = 0;

    s.ships = 0;

    s.on = 1;

    s.name[0] = '\0';
    s.id = shipno;
    putship(&s);
  }

  putrace(Race);

  planet->info[Playernum - 1].numsectsowned = 1;
  planet->explored = 0;
  planet->info[Playernum - 1].explored = 1;
  /*planet->info[Playernum-1].autorep = 1;*/

  planet->maxpopn =
      maxsupport(Race, sect, 100.0, 0) * planet->Maxx * planet->Maxy / 2;
  /* (approximate) */

  putsector(sect, planet, x, y);
  putplanet(planet, star, pnum);

  /* make star explored and stuff */
  getstar(&Stars[star], star);
  setbit(Stars[star]->explored, Playernum);
  setbit(Stars[star]->inhabited, Playernum);
  Stars[star]->AP[Playernum - 1] = 5;
  putstar(Stars[star], star);
  close_data_files();

  sigsetmask(mask);

  printf("Player %d (%s) created on sector %d,%d on %s/%s.\n", Playernum,
         race.name, x, y, Stars[star]->name, Stars[star]->pnames[pnum]);
  race.status = STATUS_ENROLLED;
  return 0;
}

/*
 * Returns: 0 if the race was successfully enrolled, or 1 if not.
 */
int enroll_player_race(failure_filename) char *failure_filename;
{
  FILE *f, *g;
  int n;
  char c[128];
  static int recursing = 0;
  static int successful_enroll_in_fix_mode = 0;

  while (critique_to_file(NULL, 1, 1)) {
    printf("Race (%s) unacceptable, for the following reason%c:\n", race.name,
           (n > 1) ? 's' : '\0');
    n = critique_to_file(stdout, 1, 1);
    if (recursing) {
      printf("\"Quit\" to break out of fix mode.\n");
      return 1;
    }

    printf(
        "Abort, enroll anyway, fix, mail rejection? [abort/enroll/fix/mail]> ");
    fgets(c, 16, stdin);
    while ((c[0] != 'a') && (c[0] != 'e') && (c[0] != 'f') && (c[0] != 'm')) {
      printf("Please choose \"abort\", \"enroll\", \"fix\", or \"mail\"> ");
      fgets(c, 16, stdin);
    }
    if (c[0] == 'e')
      break;
    if (c[0] == 'f') {
      printf("Recursive racegen.  \"Enroll\" or \"Quit\" to exit.\n");
      recursing = 1;
      modify_print_loop(1);
      please_quit = recursing = 0;
      if (successful_enroll_in_fix_mode) {
        successful_enroll_in_fix_mode = 0;
        return 0;
      }
      continue;
    }
    if (failure_filename != NULL)
      if (NULL == fopen(failure_filename, "w+")) {
        printf("Warning: unable to open failures file \"%s\".\n",
               failure_filename);
        printf("Race not saved to failures file.\n");
      } else {
        print_to_file(f, 0);
        printf("Race appended to failures file \"%s\".\n", failure_filename);
        fclose(f);
      }
    if (c[0] == 'a')
      return 1;

    g = fopen(TMP, "w");
    if (g == NULL) {
      printf("Unable to open file \"%s\".\n", TMP);
      return 1;
    }
    fprintf(g, "To: %s\n", race.address);
    fprintf(g, "Subject: %s Race Rejection\n", GAME);
    fprintf(g, "\n");
    fprintf(g, "The race you submitted (%s) was not accepted, for the "
               "following reason%c:\n",
            race.name, (n > 1) ? 's' : '\0');
    critique_to_file(g, 1, 1);
    fprintf(g, "\n");
    fprintf(g, "Please re-submit a race if you want to play in %s.\n", GAME);
    fprintf(g, "(Check to make sure you are using racegen %s)\n", VERSION);
    fprintf(g, "\n");
    fprintf(g, "For verification, here is my understanding of your race:\n");
    print_to_file(g, 1, 0);
    fclose(g);

    printf("Sending critique to %s via %s...", race.address, MAILER);
    fflush(stdout);
    sprintf(c, "cat %s | %s %s", TMP, MAILER, race.address);
    system(c);
    printf("done.\n");

    return 1;
  }

  if (enroll_valid_race())
    return enroll_player_race(failure_filename);

  if (recursing) {
    successful_enroll_in_fix_mode = 1;
    please_quit = 1;
  }

  g = fopen(TMP, "w");
  if (g == NULL) {
    printf("Unable to open file \"%s\".\n", TMP);
    return 0;
  }
  fprintf(g, "To: %s\n", race.address);
  fprintf(g, "Subject: %s Race Accepted\n", GAME);
  fprintf(g, "\n");
  fprintf(g, "The race you submitted (%s) was accepted.\n", race.name);
#if 0
  if (race.modified_by_diety) {
    fprintf(g, "The race was altered in order to be acceptable.\n") ;
    fprintf(g, "Your race now looks like this:\n") ;
    fprintf(g, "\n") ;
    print_to_file(g, verbose, 0) ;
    fprintf(g, "\n") ;
    }
#endif
  fclose(g);

  printf("Sending acceptance to %s via %s...", race.address, MAILER);
  fflush(stdout);
  sprintf(c, "cat %s | %s %s", TMP, MAILER, race.address);
  system(c);
  printf("done.\n");

  return 0;
}

int enroll(argc, argv) int argc;
char *argv[];
{
  int ret;
  FILE *g;

  if (argc < 2)
    argv[1] = DEFAULT_ENROLLMENT_FAILURE_FILENAME;
  g = fopen(argv[1], "w+");
  if (g == NULL)
    printf("Unable to open failures file \"%s\".\n", argv[1]);
  fclose(g);
  bcopy(&race, &last, sizeof(struct x));

  /*
   * race.address will be unequal to TO in the instance that this is a
   * race submission mailed from somebody other than the moderator.  */
  if (strcmp(race.address, TO))
    ret = enroll_player_race(argv[1]);
  else if (ret = critique_to_file(NULL, 1, 0)) {
    printf("Race (%s) unacceptable, for the following reason%c:\n", race.name,
           (ret > 1) ? 's' : '\0');
    critique_to_file(stdout, 1, 0);
  } else if (ret = enroll_valid_race())
    critique_to_file(stdout, 1, 0);

  if (ret)
    printf("Enroll failed.\n");
  return ret;
}

/**************
 * Iteravely loads races from a file, and enrolls them.
 */
void process(argc, argv) int argc;
char *argv[];
{
  FILE *f, *g;
  int n, nenrolled;

  if (argc < 2)
    argv[1] = DEFAULT_ENROLLMENT_FILENAME;
  f = fopen(argv[1], "r");
  if (f == NULL) {
    printf("Unable to open races file \"%s\".\n", argv[1]);
    return;
  }

  if (argc < 3)
    argv[2] = DEFAULT_ENROLLMENT_FAILURE_FILENAME;
  g = fopen(argv[2], "w");
  if (g == NULL)
    printf("Unable to open failures file \"%s\".\n", argv[2]);
  fclose(g);

  n = 0;
  nenrolled = 0;
  while (!feof(f)) {
    if (!load_from_file(f))
      continue;
    n++;
    printf("%s, from %s\n", race.name, race.address);
    /* We need the side effects: */
    last_npoints = npoints;
    npoints = STARTING_POINTS - cost_of_race();
    if (!enroll_player_race(argv[2]))
      nenrolled += 1;
  }
  fclose(f);

  printf("Enrolled %d race%c; %d failure%c saved in file %s.\n", nenrolled,
         (nenrolled != 1) ? 's' : '\0', n - nenrolled,
         (n - nenrolled != 1) ? 's' : '\0', argv[2]);
}
