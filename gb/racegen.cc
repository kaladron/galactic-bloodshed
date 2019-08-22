// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/racegen.h"

#include <unistd.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gb/enroll.h"
#include "gb/game_info.h"

static int do_racegen();

#ifdef PRIV /* Extra stuff for privileged racegen */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#endif

static int fd;
static int isserver = 0;

static int critique_modification();
static void execute(int argc, char **argv);
static void fix_up_iq();
static void initialize();
static void help(int, char *[]);
static void load(int, char *[]);
static void metamorph();
static int modify(int argc, char *argv[]);
static void normal();
static void print(int argc, char *argv[]);
static void save(int argc, char *argv[]);
static void send2(int argc, char *argv[]);
static void quit(int argc, char **argv);

int main() {
#ifdef PRIV
  int port;

  /* Check command line syntax */

  if (argc > 1) {
    if ((argv[1][0] == '-') && (isserver = (argv[1][1] == 's'))) {
      if (argc > 2)
        port = atoi(argv[2]);
      else
        port = 2020;
      if (port == 0) {
        printf("Syntax: racegen [-s [port]]\n");
        exit(0);
      }
    } else {
      printf("Syntax: racegen [-s [port]]\n");
      return (0);
    }
  }

  if (isserver) { /* Server version of racegen */
    int sockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "server: can't open stream socket");
      exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      fprintf(stderr, "server: can't bind local address");
      exit(0);
    }

    listen(sockfd, 5);

    if (fork()) {
      printf("Racegen set up on port %d\n", port);
      printf("Now accepting connections.\n\n");
      exit(0);
    }

    for (;;) {
      clilen = sizeof(cli_addr);
      fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
      if (fd < 0) fprintf(stderr, "server: accept error");
      if (fork()) {
        dup2(fd, 1);
        dup2(fd, 0);
        do_racegen();
        close(fd);
        exit(0);
      }
      close(fd);
    }
  } else
    do_racegen(); /* Non-server enroll version of racegen */

#else /* Non-PRIV version */
  do_racegen();
#endif
  return 0;
}

static attribute attr[N_ATTRIBUTES] = {
    {ADVENT,
     "Adventurism",
     0.0,
     0.0,
     0.0,
     300.0,
     0.0,
     0.05,
     0.4,
     0.99,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     0},
    {ABSORB,
     "Absorb",
     0.0,
     0.0,
     0.0,
     200.0,
     0.0,
     0.00,
     0.00,
     1.00,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     2},
    {BIRTH,
     "Birthrate",
     0.0,
     0.0,
     0.0,
     500.0,
     0.0,
     0.2,
     0.6,
     1.0,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     0},
    {COL_IQ,
     "Collective IQ",
     0.0,
     0.0,
     0.0,
     -350.5,
     0.0,
     0.00,
     0.00,
     1.00,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     2},
    {FERT,
     "Fertilize",
     200.0,
     1.0,
     1.0,
     300.0,
     0.0,
     0.0,
     0.0,
     1.0,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     0},
    {A_IQ,
     "IQ",
     100.0,
     0.03,
     140,
     6,
     0.0,
     50,
     150,
     220,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     1},
    {FIGHT,
     "Fight",
     10.0,
     0.4,
     6.0,
     65.0,
     0.0,
     1,
     4,
     20,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     1},
    {PODS,
     "Pods",
     0.0,
     0.0,
     0.0,
     200.0,
     0.0,
     0.00,
     0.00,
     1.00,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     2},
    {MASS,
     "Mass",
     100.0,
     1.0,
     3.1,
     -100.0,
     0.0,
     0.1,
     1.0,
     3.0,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     0},
    {SEXES,
     "Sexes",
     2.2,
     -0.5,
     9.0,
     -3.0,
     0.0,
     1,
     2,
     53,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     1},
    {METAB,
     "Metabolism",
     300.0,
     1,
     1.3,
     700.0,
     0.0,
     0.1,
     1.0,
     4.0,
     {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
     0}};

const char *planet_print_name[N_HOME_PLANET_TYPES] = {
    "Earth", "Forest", "Desert", "Water", "Airless", "Iceball", "Jovian"};
const int planet_cost[N_HOME_PLANET_TYPES] = {75, 50, 50, 50, -25, -25, 600};

const char *race_print_name[N_RACE_TYPES] = {"Normal", "Metamorph"};
const int race_cost[N_RACE_TYPES] = {0, 0};

const char *priv_print_name[N_PRIV_TYPES] = {"God", "Guest", "Normal"};

const char *sector_print_name[N_SECTOR_TYPES] = {
    "Water", "Land", "Mountain", "Gas", "Ice", "Forest", "Desert", "Plated"};

const int blah[N_SECTOR_TYPES] = {-1, 0, 50, 100, 200, 300, 400, 500};

const double compat_cov[N_SECTOR_TYPES][N_SECTOR_TYPES] = {
    /*  .       *        ^       ~       #       )       -      o  */
    {
        0.0,
    },
    {.001, 0.0},
    {
        .002,
        -.0005,
        0.0,
    },
    {
        999,
        999,
        999,
        0.0,
    },
    {
        .001,
        0.0,
        -.002,
        999,
        0.0,
    },
    {
        0.0,
        -.001,
        0.0,
        999,
        .001,
        0.0,
    },
    {.003, -.0005, 0.0, 999, 0.0, .001, 0.0},
    {0.0, 0.0, 0.0, 999, 0.0, 0.0, 0.0, 0.0}};

const double planet_compat_cov[N_HOME_PLANET_TYPES][N_SECTOR_TYPES] = {
    /*  .      *      ^      ~      #      )      -      o  */
    /* @ */
    {1.00, 1.00, 2.00, 99.00, 1.01, 1.50, 3.00, 1.01},
    /* ) */
    {1.01, 1.50, 2.00, 99.00, 1.01, 1.00, 3.00, 1.01},
    /* - */
    {3.00, 1.01, 1.01, 99.00, 1.50, 3.00, 1.00, 1.01},
    /* . */
    {1.00, 1.50, 3.00, 99.00, 1.01, 1.01, 3.00, 1.01},
    /* O */
    {1.01, 1.00, 1.00, 99.00, 1.01, 1.01, 1.00, 1.01},
    /* # */
    {3.00, 1.01, 1.00, 99.00, 1.00, 1.50, 2.00, 1.01},
    /* ~ */
    {99.00, 99.00, 99.00, 1.00, 99.00, 99.00, 99.00, 99.00}};

/**************
 * Global variables for this program.
 */
struct x race_info, cost_info, last;

int npoints = STARTING_POINTS;
int last_npoints = STARTING_POINTS;
int altered = 0;     /* 1 iff race has been altered since last saved */
int changed = 1;     /* 1 iff race has been changed since last printed */
int please_quit = 0; /* 1 iff you want to exit ASAP. */

/**************
 * Price function for racegen.  Finds the cost of `race', and returns it.
 * As a side effect the costs for all the individual fields of `race' get
 * stuffed into `cost' for later printing.
 */
int cost_of_race() {
  int i;
  int j;
  int sum = 0;

#define ROUND(f) ((int)(0.5 + (f)))
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    cost_info.attr[i] =
        (exp(attr[i].e_fudge * (race_info.attr[i] - attr[i].e_hinge)) *
             attr[i].e_factor +
         race_info.attr[i] * attr[i].l_factor);
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    for (j = FIRST_ATTRIBUTE; j <= LAST_ATTRIBUTE; j++)
      if (attr[i].cov[j] != 0.0)
        cost_info.attr[i] *= (1.0 + attr[i].cov[j] * race_info.attr[j]);
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    sum += (cost_info.attr[i] = ROUND(cost_info.attr[i] + attr[i].l_fudge));

  sum += (cost_info.home_planet_type = planet_cost[race_info.home_planet_type]);
  sum += (cost_info.race_type = race_cost[race_info.race_type]);
  race_info.n_sector_types = 0;
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++) {
    if (race_info.compat[i] != 0.0) race_info.n_sector_types += 1;
    /* Get the base costs: */
    cost_info.compat[i] =
        race_info.compat[i] * 0.5 + 10.8 * log(1.0 + race_info.compat[i]);
  }
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++)
    for (j = i + 1; j <= LAST_SECTOR_TYPE; j++)
      if (compat_cov[j][i] != 0.0) {
        cost_info.compat[i] *= (1.0 + compat_cov[j][i] * race_info.compat[j]);
        cost_info.compat[j] *= (1.0 + compat_cov[j][i] * race_info.compat[i]);
      }
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++)
    if (planet_compat_cov[race_info.home_planet_type][i] > 1.01)
      cost_info.compat[i] *= planet_compat_cov[race_info.home_planet_type][i];
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++)
    sum += (cost_info.compat[i] = ROUND(cost_info.compat[i]));
  sum += (cost_info.n_sector_types = blah[race_info.n_sector_types]);
  return sum;
}

static void metamorph() {
  /* Adventurousness is not correlated with Max IQ. */
  attr[ADVENT].cov[A_IQ] = 0.00;
  /* A high birthrate is easier if few sexes, high metab, and low mass.*/
  /** Morphs are not born, they hatch.  THus the mass limitation is smaller:**/
  attr[BIRTH].cov[MASS] = 0.10 / ATTR_RANGE(MASS);
  attr[BIRTH].cov[SEXES] = 0.50 / ATTR_RANGE(SEXES);
  attr[BIRTH].cov[METAB] = -0.10 / ATTR_RANGE(METAB);
  /* Natural fertilization is ultimately associated with metabolic activity */
  /** Morphs are naturally adept at fertilization. **/
  attr[FERT].cov[METAB] = -0.30 / ATTR_RANGE(METAB);
  /* Fighting is easier for independent, high-mass, high-metab races:   */
  attr[FIGHT].cov[ADVENT] = -0.10 / ATTR_RANGE(ADVENT);
  attr[FIGHT].cov[MASS] = -0.20 / ATTR_RANGE(MASS);
  attr[FIGHT].cov[METAB] = -0.15 / ATTR_RANGE(METAB);
  /* A high metabolism is hard to support if you have high mass: */
  /** Due to general squishiness, this effect is not as strong for mesos. **/
  attr[METAB].cov[MASS] = 0.05 / ATTR_RANGE(MASS);
  /* However, a large IQ is easier with high mass; (lot of brain space): */
  /** IQ represents max IQ, thus, no go. **/
  attr[A_IQ].cov[MASS] = 0.00;
  /* IQ is more expensive due to collective intelligence: */
  attr[A_IQ].cov[COL_IQ] = 0.00 / ATTR_RANGE(COL_IQ);

  strcpy(attr[A_IQ].print_name, "IQ Limit");
}

static void normal() {
  /* Adventurousness is more likely with people smart enough to do it. */
  attr[ADVENT].cov[A_IQ] = -0.40 / ATTR_RANGE(A_IQ);
  /* Birthrate is easier if few sexes, high metab, low iq, and low mass.*/
  attr[BIRTH].cov[A_IQ] = 0.20 / ATTR_RANGE(A_IQ);
  attr[BIRTH].cov[MASS] = 0.40 / ATTR_RANGE(MASS);
  attr[BIRTH].cov[SEXES] = 0.90 / ATTR_RANGE(SEXES);
  attr[BIRTH].cov[METAB] = -0.20 / ATTR_RANGE(METAB);
  /* Natural fertilization is ultimately associated with metabolic activity */
  attr[FERT].cov[METAB] = -0.20 / ATTR_RANGE(METAB);
  /* Fighting is easier for independent, high-mass, high-metab races:   */
  attr[FIGHT].cov[A_IQ] = -0.20 / ATTR_RANGE(A_IQ);
  attr[FIGHT].cov[ADVENT] = -0.05 / ATTR_RANGE(ADVENT);
  attr[FIGHT].cov[MASS] = -0.20 / ATTR_RANGE(MASS);
  attr[FIGHT].cov[METAB] = -0.05 / ATTR_RANGE(METAB);
  /* A high metabolism is hard to support if you have high mass: */
  attr[METAB].cov[MASS] = 0.15 / ATTR_RANGE(MASS);
  attr[METAB].cov[A_IQ] = -0.10 / ATTR_RANGE(A_IQ);
  /* However, a large IQ is easier with high mass; (lot of brain space): */
  attr[A_IQ].cov[MASS] = -0.25 / ATTR_RANGE(MASS);

  strcpy(attr[A_IQ].print_name, "IQ");
}

static void fix_up_iq() {
  if (race_info.attr[COL_IQ] == 1.0)
    strcpy(attr[A_IQ].print_name, "IQ Limit");
  else
    strcpy(attr[A_IQ].print_name, "IQ");
}

/**************
 * VERY IMPORTANT FUNCTION: this function is a representation function
 * for the race datatype.  That is, it will return a positive integer iff
 * the race is NOT valid.  It is thus useful both when modifying a race and
 * when loading a race.  The file f should be nullptr if you do not want to
 * print out descriptions of the errors; otherwise, error message(s) will be
 * printed to that file.
 */
int critique_to_file(FILE *f, int rigorous_checking, int is_player_race) {
  int i;
  int nerrors = 0;

#define FPRINTF \
  if (f != nullptr) fprintf

  /*
   * Check for valid attributes: */
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++) {
    if ((attr[i].is_integral == 2) && (race_info.attr[i] != 0.0) &&
        (race_info.attr[i] != 1.0)) {
      FPRINTF(f, "%s is boolean valued.  Use \"yes\" or \"no\".\n",
              attr[i].print_name);
      nerrors += 1;
    }
    if (race_info.attr[i] < attr[i].minimum) {
      FPRINTF(f, "%s must be at least %0.2f.\n", attr[i].print_name,
              attr[i].minimum);
      nerrors += 1;
    }
    if (race_info.attr[i] > attr[i].maximum) {
      FPRINTF(f, "%s may be at most %0.2f.\n", attr[i].print_name,
              attr[i].maximum);
      nerrors += 1;
    }
    /* Warning, but no error: */
    if (attr[i].is_integral) {
      if (race_info.attr[i] != ((double)((int)race_info.attr[i])))
        FPRINTF(f, "%s is integral; truncated to %1.0f.\n", attr[i].print_name,
                race_info.attr[i] = ((double)((int)race_info.attr[i])));
    } else if (race_info.attr[i] !=
               (((double)((int)(100.0 * race_info.attr[i]))) / 100.0))
      FPRINTF(f, "%s truncated to next lowest hundredth (%1.2f).\n",
              attr[i].print_name,
              race_info.attr[i] =
                  (((double)((int)(100.0 * race_info.attr[i]))) / 100.0));
  }

  /*
   * Check for valid normal race attributes: */
  if (race_info.race_type == R_NORMAL) {
    if (race_info.attr[ABSORB] != 0.0) {
      FPRINTF(f, "Normal races do not absorb their enemies in combat.\n");
      nerrors += 1;
    }
    if (race_info.attr[PODS] != 0.0) {
      FPRINTF(f, "Normal races do not make pods.\n");
      nerrors += 1;
    }
  }

  /*
   * Check for valid name:  */
  if (0 == strlen(race_info.name)) {
    FPRINTF(f, "Use a non-empty name.\n");
    nerrors += 1;
  }

  /*
   * Check for valid privileges:  */
  if ((race_info.priv_type < FIRST_PRIV_TYPE) ||
      (race_info.priv_type > LAST_PRIV_TYPE)) {
    FPRINTF(f, "Privileges out of valid range.\n");
    nerrors += 1;
  }
  if ((race_info.priv_type != P_NORMAL) && is_player_race) {
    FPRINTF(f, "Players may not create %s races.\n",
            priv_print_name[race_info.priv_type]);
    nerrors += 1;
  }

  /*
   * Check for valid home planet: */
  if ((race_info.home_planet_type < FIRST_HOME_PLANET_TYPE) ||
      (race_info.home_planet_type > LAST_HOME_PLANET_TYPE)) {
    FPRINTF(f, "Home planet type out of valid range.\n");
    nerrors += 1;
  }

  /*
   * Check for valid race: */
  if ((race_info.race_type < FIRST_RACE_TYPE) ||
      (race_info.race_type > LAST_RACE_TYPE)) {
    FPRINTF(f, "Home planet type out of valid range.\n");
    nerrors += 1;
  }

  /*
   * Check for valid sector compats: */
  if ((race_info.home_planet_type != H_JOVIAN) &&
      (race_info.compat[S_PLATED] != 100.0)) {
    FPRINTF(f, "Non-jovian races must have 100%% plated compat.\n");
    nerrors += 1;
  }
  for (i = FIRST_SECTOR_TYPE + 1; i <= LAST_SECTOR_TYPE; i++) {
    if (race_info.compat[i] < 0.0) {
      FPRINTF(f, "Sector compatibility is at minimum 0%%.\n");
      nerrors += 1;
    }
    if (race_info.compat[i] > 100.0) {
      FPRINTF(f, "Sector compatibility may be at most 100%%.\n");
      nerrors += 1;
    }
    if ((i == S_GAS) && (race_info.compat[i] != 0.0) &&
        (race_info.home_planet_type != H_JOVIAN)) {
      FPRINTF(f, "Non-jovian races may never have gas compatibility!\n");
      nerrors += 1;
    }
    if ((i != S_GAS) && (race_info.compat[i] != 0.0) &&
        (race_info.home_planet_type == H_JOVIAN)) {
      FPRINTF(f, "Jovian races may have no compatibility other than gas!\n");
      nerrors += 1;
    }
    /* A warning, but no error: */
    if (race_info.compat[i] != ((double)((int)race_info.compat[i])))
      FPRINTF(f, "Sector compatibilities are integral; truncated to %1.0f.\n",
              race_info.compat[i] = ((double)((int)race_info.compat[i])));
  }

  if (rigorous_checking) {
    /*
     * Any rejection notice is an error: */
    if (strlen(race_info.rejection)) {
      FPRINTF(f, "%s", race_info.rejection);
      nerrors += 1;
    }
    /*
     * Check for valid password: */
    if (MIN_PASSWORD_LENGTH > strlen(race_info.password)) {
      FPRINTF(f, "Passwords are required to be at least %d characters long.\n",
              MIN_PASSWORD_LENGTH);
      nerrors += 1;
    } else if (!strcmp(race_info.password, "XXXX")) {
      FPRINTF(f, "You must change your password from the default.\n");
      nerrors += 1;
    }
    if (!strcmp(race_info.address, "Unknown")) {
      FPRINTF(f, "You must change your email address.\n");
      nerrors += 1;
    }
    /*
     * Check that race isn't 'superrace': */
    if (npoints < 0) {
      FPRINTF(f, "You can't have negative points left!\n");
      nerrors += 1;
    }
    /*
     * Check that sector compats are reasonable. */
    if ((race_info.home_planet_type != H_JOVIAN) &&
        (race_info.n_sector_types == 1)) {
      FPRINTF(f,
              "Non-jovian races must be compat with at least one sector "
              "type besides plated.\n");
      nerrors += 1;
    }
    for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++)
      if ((planet_compat_cov[race_info.home_planet_type][i] == 1.0) &&
          (race_info.compat[i] == 100.0))
        break;
    if (i == N_SECTOR_TYPES) {
      FPRINTF(f,
              "You must have 100%% compat with at least one sector type "
              "that is common on\n  your home planet type.  (Marked with a "
              "'*')\n");
      nerrors += 1;
    }
  }
  if (race_info.status >= 0)
    race_info.status = (nerrors == 0) ? STATUS_BALANCED : STATUS_UNBALANCED;
  return nerrors;
#undef FPRINTF
}

static int critique_modification() {
  int nerrors;

  race_info.rejection[0] = '\0';
  nerrors = critique_to_file(stdout, 0, IS_PLAYER);
  if (nerrors)
    bcopy(&last, &race_info, sizeof(struct x));
  else
    changed = altered = 1;
  race_info.status = (nerrors == 0) ? STATUS_BALANCED : STATUS_UNBALANCED;
  return nerrors;
}

/**************
 * Initialize the race to the init value, and set the l_fudge values
 * accordingly so that the cost of this race's attributes is zero.
 */
static void initialize() {
  int i;

  bzero(&race_info, sizeof(race_info));
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    race_info.attr[i] = attr[i].init;
  race_info.race_type = R_NORMAL;
  race_info.priv_type = P_NORMAL;
  race_info.home_planet_type = H_EARTH;
  race_info.n_sector_types = 1;
  race_info.compat[S_PLATED] = 100;
  strcpy(race_info.name, "Unknown");
  strcpy(race_info.address, "Unknown");
  strcpy(race_info.password, "XXXX");
  normal();
  bcopy(&race_info, &last, sizeof(struct x));
  cost_of_race();
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    attr[i].l_fudge += -cost_info.attr[i];
  cost_of_race();
#ifdef ENROLL
  init_enroll();
#endif
}

/**************
 * Now the functions that implement racegen's commands.  Rather than me
 * trying to tell you about them here, just run the program and diddle
 * with it to get the idea.
 */
static void help(int argc, char *argv[]) {
  int enroll;
  int process;
  int i;
  int j;
  int helpp;
  int load;
  int modify;
  int print;
  int save;
  int send2;
  int quit;

  if (argc == 1) {
    enroll = process = helpp = load = modify = print = save = send2 = quit = 1;
    printf("\n");
    printf(
        "To execute a command, type it at the command line.  All commands\n");
    printf("and command arguments maybe either upper or lower case, and/or\n");
    printf("abbreviated.  The available commands are:\n");
  } else {
    enroll = process = helpp = load = modify = print = save = send2 = quit = 0;
    for (i = 1; i < argc; i++) {
      j = strlen(argv[i]);
#ifdef PRIV
      if (!strncasecmp(argv[i], "enroll", j) && (!isserver))
        enroll = 1;
      else
#endif
          if (!strncasecmp(argv[i], "help", j))
        helpp = 1;
      else if (!strncasecmp(argv[i], "load", j) && (!isserver))
        load = 1;
      else if (!strncasecmp(argv[i], "modify", j))
        modify = 1;
      else if (!strncasecmp(argv[i], "print", j))
        print = 1;
#ifdef PRIV
      else if (!strncasecmp(argv[i], "process", j) && (!isserver))
        process = 1;
#endif
      else if (!strncasecmp(argv[i], "save", j) && (!isserver))
        save = 1;
      else if (!strncasecmp(argv[i], "send", j))
        send2 = 1;
      else if (!strncasecmp(argv[i], "quit", j))
        quit = 1;
      else {
        printf("\n");
        printf("\"%s\" is not a command.\n", argv[i]);
      }
    }
  }

#ifdef PRIV
  if (enroll) {
    printf("\n");
    printf("Enroll\n");
    printf("\t\t This command will add the current race to the game,\n");
    printf("\t\t after checking to make sure it has all points spent, and\n");
    printf("\t\t other such administrivia.\n");
  }
#endif

  if (helpp) {
    printf("\n");
    printf("Help [command]*\n");
    printf("\t\t This command gives you information about racegen's\n");
    printf("\t\t commands.  If called with no arguments, it will print\n");
    printf("\t\t information about all available commands.  Otherwise it\n");
    printf("\t\t only prints information about the specified commands.\n");
  }

  if (load) {
    printf("\n");
    printf("Load [filename]\n");
    printf("\t\t This command will load your race from the file specified\n");
    printf("\t\t in the optional first argument, or from the file \"%s\"\n",
           (race_info.filename[0] ? race_info.filename : SAVETO));
    printf("\t\t if no argument is given.\n");
  }

  if (modify) {
    printf("\n");
    printf("Modify arg1 arg2\n");
    printf("\t\t This command allows you to change the values of your\n");
    printf(
        "\t\t race's name, password, type, attributes, planet, and compats.\n");
    printf("\t\t The syntax is as follows:\n");
    printf("\t\t   <modify>     ::= modify <attr> <value>\n");
    printf("\t\t                  | modify address <string>\n");
    printf("\t\t                  | modify name <string>\n");
    printf("\t\t                  | modify password <string>\n");
    printf("\t\t                  | modify planet <planettype>\n");
#ifdef PRIV
    printf("\t\t                  | modify privilege <privtype>\n");
#endif
    printf("\t\t                  | modify race <racetype>\n");
    printf("\t\t                  | modify <sectortype> <value>\n");

    printf("\t\t   <attribute>  ::= %s", attr[0].print_name);
    for (i = FIRST_ATTRIBUTE + 1; i < LAST_ATTRIBUTE; i++) {
      printf(" | %s", attr[i].print_name);
      if ((i % 3) == 2) printf("\n\t\t                 ");
    }
    printf("\n");

    printf("\t\t   <planettype> ::= %s", planet_print_name[0]);
    for (i = FIRST_HOME_PLANET_TYPE + 1;
         i <= std::min(4, LAST_HOME_PLANET_TYPE); i++) {
      printf(" | %s", planet_print_name[i]);
    }
    printf("\n\t\t                 ");
    for (; i <= LAST_HOME_PLANET_TYPE; i++) {
      printf(" | %s", planet_print_name[i]);
    }
    printf("\n");

    printf("\t\t   <racetype>   ::= %s", race_print_name[0]);
    for (i = FIRST_RACE_TYPE + 1; i <= LAST_RACE_TYPE; i++) {
      printf(" | %s", race_print_name[i]);
    }
    printf("\n");

    printf("\t\t   <sectortype> ::= %s", sector_print_name[1]);
    for (i = FIRST_SECTOR_TYPE + 2; i <= std::min(5, LAST_SECTOR_TYPE); i++) {
      printf(" | %s", sector_print_name[i]);
    }
    printf("\n\t\t                 ");
    for (; i <= LAST_SECTOR_TYPE; i++) {
      printf(" | %s", sector_print_name[i]);
    }
    printf("\n");
  }

  if (print) {
    printf("\n");
    printf("Print [filename]\n");
    printf("\t\t With no argument, this command prints your race to the\n");
    printf("\t\t screen.  It is automatically executed after each modify.\n");
    printf("\t\t Otherwise it saves a text copy of your race to the file\n");
    printf("\t\t specified in the first argument.\n");
  }

#ifdef PRIV
  if (process) {
    printf("\n");
    printf("Process filename\n");
    printf("\t\t This command will repeatedly load races from filename,\n");
    printf("\t\t and then try to enroll them.  You can thus easily \n");
    printf("\t\t enroll tens of players at once.  \n");
  }
#endif

  if (save) {
    printf("\n");
    printf("Save [filename]\n");
    printf("\t\t This command will save your race to the file specified in\n");
    printf("\t\t the optional first argument, or to the file \"%s\"\n",
           (race_info.filename[0] ? race_info.filename : SAVETO));
    printf("\t\t if no argument is given.\n");
  }

  if (send2) {
    printf("\n");
    printf("Send\n");
    printf("\t\t This command will send your race to God, (%s).\n", TO);
    printf("\t\t It will not work unless you have spent all your points.\n");
  }

  if (quit) {
    printf("\n");
    printf("Quit\n");
    printf("\t\t This command will prompt you to save your work if you\n");
    printf("\t\t haven't already, and then exit this program.\n");
  }

  printf("\n");
}

/*
 * Return non-zero on failure, zero on success. */
int load_from_file(FILE *g) {
  int i;
  char buf[80];
  char from_address[80];

#define FSCANF(file, format, variable) \
  if (EOF == fscanf((file), (format), (variable))) goto premature_end_of_file

  do {
    FSCANF(g, " %s", buf);
    if (0 == strcmp(buf, "From:")) {
      FSCANF(g, " %s", buf);
      strcpy(from_address, buf);
    }
  } while (strcmp(buf, START_RECORD_STRING));

  race_info.status = STATUS_BALANCED;
  FSCANF(g, " %s", race_info.address);
  FSCANF(g, " %s", race_info.name);
  FSCANF(g, " %s", race_info.password);
  FSCANF(g, " %d", &race_info.priv_type);
  FSCANF(g, " %d", &race_info.home_planet_type);
  FSCANF(g, " %d", &race_info.race_type);
  if (race_info.race_type == R_NORMAL)
    normal();
  else
    metamorph();
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    FSCANF(g, " %lf", &race_info.attr[i]);
  fix_up_iq();
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++)
    FSCANF(g, " %lf", &race_info.compat[i]);
  do {
    FSCANF(g, " %s", buf);
  } while (strcmp(buf, END_RECORD_STRING));
  return 0;
premature_end_of_file:
  printf("Error: premature end of file.\n");
  return 1;
}

/*
 * Return non-zero on failure, zero on success. */

static int load_from_filename(const char *filename) {
  int ret;
  FILE *f = fopen(filename, "r");

  if (f == nullptr) {
    printf("Unable to open file \"%s\".\n", filename);
    return 1;
  }
  ret = load_from_file(f);
  fclose(f);
  return ret;
}

static void load(int argc, char *argv[]) {
  char c[64];
  int i;

  bcopy(&race_info, &last, sizeof(struct x));
  if (altered) {
    i = Dialogue("This race has been altered; load anyway?", "yes", "no", 0);
    if (i == 1) return;
  }
  if (argc > 1)
    strcpy(c, argv[1]);
  else if (!race_info.filename[0])
    strcpy(c, SAVETO);
  if (load_from_filename(c)) {
    printf("Load from file \"%s\" failed.\n", c);
    bcopy(&last, &race_info, sizeof(struct x));
  } else {
    printf("Loaded race from file \"%s\".\n", c);
    strcpy(race_info.filename, c);
    altered = 0;
    changed = 1;
  }
}

static int modify(int argc, char *argv[]) {
  int i;
  int j;
  static char *help_strings[2] = {nullptr, "modify"};
  double f;

  if (argc < 3) {
    help(2, help_strings);
    return -1;
  }
  j = strlen(argv[1]);

  bcopy(&race_info, &last, sizeof(struct x));

  /*
   * Check for attribute modification: */
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++)
    if (!strncasecmp(argv[1], attr[i].print_name, j)) {
      if (attr[i].is_integral == 2) { /* Boolean attribute. */
        j = strlen(argv[2]);
        if (!strncasecmp(argv[2], "no", j))
          f = 0.0;
        else if (!strncasecmp(argv[2], "yes", j))
          f = 1.0;
        else
          f = atof(argv[2]);
      } else
        f = atof(argv[2]);

      race_info.attr[i] = f;
      fix_up_iq();
      return critique_modification();
    }

  /*
   * Check for name modification:  */
  if (!strncasecmp(argv[1], "name", j)) {
    strcpy(race_info.name, argv[2]);
    return critique_modification();
  }

  /*
   * Check for from-address modification:  */
  if (!strncasecmp(argv[1], "address", j)) {
    strcpy(race_info.address, argv[2]);
    return critique_modification();
  }

#ifdef PRIV
  /*
   * Check for privilege modification:  */
  if (!strncasecmp(argv[1], "privilege", j)) {
    j = strlen(argv[2]);
    for (i = FIRST_PRIV_TYPE; i <= LAST_PRIV_TYPE; i++)
      if (!strncasecmp(argv[2], priv_print_name[i], j)) {
        race_info.priv_type = i;
        return critique_modification();
      }
    race_info.priv_type = atof(argv[2]);
    return critique_modification();
  }
#endif

  /*
   * Check for planet modification:  */
  if (!strncasecmp(argv[1], "planet", j)) {
    j = strlen(argv[2]);
    for (i = FIRST_HOME_PLANET_TYPE; i <= LAST_HOME_PLANET_TYPE; i++)
      if (!strncasecmp(argv[2], planet_print_name[i], j)) {
        if (i == H_JOVIAN) {
          bzero(race_info.compat, sizeof(race_info.compat));
          race_info.compat[S_GAS] = 100.0;
        } else if (race_info.home_planet_type == H_JOVIAN) {
          race_info.compat[S_PLATED] = 100.0;
          race_info.compat[S_GAS] = 0.0;
        }
        race_info.home_planet_type = i;
        return critique_modification();
      }
    printf("\"%s\" is not a valid planet type.\n", argv[2]);
    return -1;
  }

  /*
   * Check for password modification:  */
  if (!strncasecmp(argv[1], "password", j)) {
    strcpy(race_info.password, argv[2]);
    return critique_modification();
  }

  /*
   * Check for race modification:  */
  if (!strncasecmp(argv[1], "race", j)) {
    j = strlen(argv[2]);
    for (i = FIRST_RACE_TYPE; i <= LAST_RACE_TYPE; i++)
      if (!strncasecmp(argv[2], race_print_name[i], j)) {
        if (i == R_METAMORPH) {
          race_info.attr[ABSORB] = 1;
          race_info.attr[PODS] = 1;
          race_info.attr[COL_IQ] = 1;
          metamorph();
        } else {
          race_info.attr[ABSORB] = 0;
          race_info.attr[PODS] = 0;
          race_info.attr[COL_IQ] = 0;
          normal();
        }
        race_info.race_type = i;
        return critique_modification();
      }
    printf("\"%s\" is not a valid race type.\n", argv[2]);
    return -1;
  }

  /*
   * Check for sector_type modification: */
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++)

#ifndef PRIV
    if (i == S_PLATED)
      continue; /* Players should never need to modify this. */
    else
#endif
        if (!strncasecmp(argv[1], sector_print_name[i], j)) {
      race_info.compat[i] = atof(argv[2]);
      return critique_modification();
    }

  /*
   * Print error */
  printf("\n");
  printf("Modify: didn't recognize the first argument \"%s\".\n", argv[1]);
  printf("Type \"help modify\" for more information on modify.\n");
  printf("\n");
  return -1;
}

void print_to_file(FILE *f, int verbose) {
#define FPRINTF \
  if (verbose) fprintf
  int i;

  if (!verbose) fprintf(f, START_RECORD_STRING);

  FPRINTF(f, "\nAddress  :");
  fprintf(f, " %s", race_info.address);
  FPRINTF(f, "\n");

  FPRINTF(f, "Name     :");
  fprintf(f, " %s", race_info.name);
  FPRINTF(f, "\n");

  FPRINTF(f, "Password :");
  fprintf(f, " %s", race_info.password);
  FPRINTF(f, "\n");

#ifdef PRIV
  FPRINTF(f, "Privileges:");
  if (verbose)
    fprintf(f, "%11.11s", priv_print_name[race_info.priv_type]);
  else
#endif
      if (!verbose)
    fprintf(f, " %d", race_info.priv_type);

#ifdef PRIV
  FPRINTF(f, "\n");
#endif

  FPRINTF(f, "Planet   :");
  if (verbose)
    fprintf(f, " %s", planet_print_name[race_info.home_planet_type]);
  else
    fprintf(f, " %d", race_info.home_planet_type);
  FPRINTF(f, "  [%4d]\n", cost_info.home_planet_type);

  FPRINTF(f, "Race type:");
  if (verbose)
    fprintf(f, " %s", race_print_name[race_info.race_type]);
  else
    fprintf(f, " %d", race_info.race_type);
  FPRINTF(f, "  [%4d]\n", cost_info.race_type);
  FPRINTF(f, "\n");

  FPRINTF(f, "Attributes:\n");
  for (i = FIRST_ATTRIBUTE; i <= LAST_ATTRIBUTE; i++) {
    FPRINTF(f, "%13.13s:", attr[i].print_name);
    if (verbose && (attr[i].is_integral == 2))
      fprintf(f, (race_info.attr[i] > 0.0) ? "  yes   " : "   no   ");
    else
      fprintf(f, " %7.2f", race_info.attr[i]);
    FPRINTF(f, "  [%4.0f]", cost_info.attr[i]);
    FPRINTF(f, (i & 01) ? "\n" : "     ");
  }
  if (i & 01) FPRINTF(f, "\n");
  FPRINTF(f, "\n");

  FPRINTF(f, "Sector Types:    %2d     [%4d]\n", race_info.n_sector_types,
          cost_info.n_sector_types);
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++) {
    FPRINTF(f, "%13.13s: ", sector_print_name[i]);
    fprintf(f, " %3.0f", race_info.compat[i]);
    FPRINTF(
        f, "%%   %c[%4.0f]",
        (planet_compat_cov[race_info.home_planet_type][i] == 1.0) ? '*' : ' ',
        cost_info.compat[i]);
    FPRINTF(f, (i & 01) ? "\n" : "     ");
  }

  if (!verbose) fprintf(f, END_RECORD_STRING);
  FPRINTF(f, "\n");
  FPRINTF(f, "Points left: %d          Previous value: %d\n", npoints,
          last_npoints);
  fprintf(f, "\n");
#undef FPRINTF
}

static int print_to_filename(const char *filename, int verbose) {
  FILE *f = fopen(filename, "w");

  if (f == nullptr) {
    printf("Unable to open file \"%s\".\n", filename);
    return 0;
  }
  print_to_file(f, verbose);
  fclose(f);
  return 1;
}

static void print(int argc, char *argv[]) {
  if (argc == 1)
    print_to_file(stdout, 1);
  else if (print_to_filename(argv[1], 1))
    printf("Printed race to file \"%s\".\n", argv[1]);
}

static void save(int argc, char *argv[]) {
  if (argc > 1)
    strcpy(race_info.filename, argv[1]);
  else if (!race_info.filename[0])
    strcpy(race_info.filename, SAVETO);
  if (print_to_filename(race_info.filename, 0)) {
    printf("Saved race to file \"%s\".\n", race_info.filename);
    altered = 0;
  }
}

static void send2(int, char **) {
  FILE *f;
  char sys[64];

  bcopy(&race_info, &last, sizeof(struct x));
  if (critique_to_file(stdout, 1, IS_PLAYER)) return;

  f = fopen(race_info.password, "w");
  if (f == nullptr) {
    printf("Unable to open file \"%s\".\n", race_info.password);
    return;
  }
  fprintf(f, "From: %s\n", race_info.address);
  fprintf(f, "Subject: %s Race Submission\n", GAME);
  fprintf(f, "\n");
  print_to_file(f, 0);
  fclose(f);

  fflush(stdout);
  printf("Mailing race to %s : ", TO);
  sprintf(sys, "cat %s | %s %s", race_info.password, MAILER, TO);
  if (system(sys) < 0) {
    perror("gaaaaaaah");
    exit(-1);
  }
  printf("done.\n");

  f = fopen(race_info.password, "w");
  if (f == nullptr) {
    printf("Unable to open file \"%s\".\n", race_info.password);
    return;
  }
  fprintf(f, "From: %s\n", race_info.address);
  fprintf(f, "Subject: %s Race Submission\n\n", GAME);
  print_to_file(f, 1);
  fclose(f);

  fflush(stdout);
  printf("Mailing race to %s : ", race_info.address);
  sprintf(sys, "cat %s | %s %s", race_info.password, MAILER, race_info.address);
  if (system(sys) < 0) {
    perror("gaaaaaaah");
    exit(-1);
  }
  printf("done.\n");
  unlink(race_info.password);
}

int Dialogue(const char *prompt, ...) {
  va_list ap;
#define INPUTSIZE 512
  char input[INPUTSIZE];
  char *carg;
  int len;
  int i;
  int argc = 0;
  int init = 0;
  char *argv[16];
  printf("%s", prompt);
  va_start(ap, prompt);
  while ((carg = va_arg(ap, char *)) != nullptr) {
    if (!init) {
      printf(" [%s", carg);
      init = 1;
    } else {
      printf("/%s", carg);
    }
    argv[argc++] = carg++;
  }
  va_end(ap);
  if (argc) printf("]");
  printf("> ");
  fflush(stdout);
  while (fgets(input, INPUTSIZE, stdin) != nullptr) {
    if (argc == 0) return -1;
    len = strlen(input) - 1;

    for (i = 0; i < argc; i++)
      if (!strncasecmp(argv[i], input, len)) return i;
    /*
     * The input did not match any of the valid responses: */
    printf("Please enter ");
    for (i = 0; i < argc - 1; i++) {
      printf("\"%s\", ", argv[i]);
    }
    printf("or \"%s\"> ", argv[i]);
  }
  return 0;
}

static void quit(int, char **) {
  int i;

  if (please_quit) { /* This could happen if ^c is hit while here. */
    if (isserver) close(fd);
    exit(0);
  }
  please_quit = 1;
  if (altered) {
    if (!isserver) {
      i = Dialogue("Save this race before quitting?", "yes", "no", "abort", 0);
      if (i == 0)
        save(1, nullptr);
      else if (i == 2)
        please_quit = 0;
    } else {
      i = Dialogue("Are you sure?", "yes", "no", "abort", 0);
      if (i == 1) please_quit = 0;
    }
  }
}

/**************
 * This function merely takes the space-parsed command line and executes
 * one of the commands above.
 */
static void execute(int argc, char **argv) {
  int i;

#if 0
  for (i = 0; i < argc; i++)
    printf ("%d: \"%s\"\n", i, argv[i]);
#endif
  if (argc == 0) {
    printf("Type \"help\" for help.\n");
    return;
  }
  i = strlen(argv[0]);
#ifdef PRIV
  if (!strncasecmp(argv[0], "enroll", i) && !isserver)
    enroll(argc, argv);
  else
#endif
      if (!strncasecmp(argv[0], "help", i))
    help(argc, argv);
  else if (!strncasecmp(argv[0], "load", i) && !isserver)
    load(argc, argv);
  else if (!strncasecmp(argv[0], "modify", i))
    modify(argc, argv);
  else if (!strncasecmp(argv[0], "print", i))
    print(argc, argv);
#ifdef PRIV
  else if (!strncasecmp(argv[0], "process", i) && !isserver)
    process(argc, argv);
#endif
  else if (!strncasecmp(argv[0], "save", i) && !isserver)
    save(argc, argv);
  else if (!strncasecmp(argv[0], "send", i))
    send2(argc, argv);
  else if (!strncasecmp(argv[0], "quit", i))
    quit(argc, argv);
  else {
    printf("Unknown command \"%s\".  Type \"help\" for help.\n", argv[0]);
    return;
  }
}

/**************
 * Here is the main loop, where I print the command prompt, parse it into
 * words using spaces as the separator, and call execute.  Level is the
 * number of higher level modify print loops above this one.  It will
 * always be zero for player racegens.
 */
void modify_print_loop(int) {
#define BUFSIZE 512
  char buf[BUFSIZE];
  char *com;
  char *args[4];
  int i;

  while (!please_quit) {
    last_npoints = npoints;
    npoints = STARTING_POINTS - cost_of_race();

    if (changed) {
      print_to_file(stdout, 1);
      changed = 0;
    }
#ifdef PRIV
    if (isserver)
      printf("Command [help/modify/print/send/quit]> ");
    else
      printf("%s [enroll/help/load/modify/print/process/save/send/quit]> ",
             level ? "Fix" : "Command");
#else
    printf("Command [help/load/modify/print/save/send/quit]> ");
#endif
    fflush(stdout);
    com = fgets(buf, BUFSIZE, stdin);
    buf[strlen(buf) - 1] = '\0';

    for (i = 0; i < 4; i++) {
      while (*com && (*com == ' ')) *com++ = '\0';
      if (!*com) break;
      args[i] = com;
      while (*com && (*com != ' ')) com++;
    }
    execute(i, args);
  }
  printf("\n");
}

/**************
 * Print out initial info and then call modify-print loop.
 */
static int do_racegen() {
  initialize();
  printf("\n");
  printf("Galactic Bloodshed Race Generator %s\n", VERSION);
  printf("\n");
  printf("Finished races will be sent to %s.\n", TO);
  printf("***************************************************************\n");
  printf("Game: %s, using %s\n", GAME, GB_VERSION);
  printf("Address: %s\n", LOCATION);
  printf("God: %s\n", MODERATOR);
  printf("Starts: %s\n", STARTS);
  printf("Stars: %s; Players: %s\n", STARS, PLAYERS);
  printf("Any race may be refused or modified by God for any reason!\n");
  printf("\n");
  printf("DEADLINE: %s\n", DEADLINE);
  printf("***************************************************************\n");
  printf("Update schedule:\n");
  printf("%s\n", UPDATE_SCH);
  printf("\n");
  printf("If you cannot make this update schedule, do NOT send in a race!\n");
  printf("***************************************************************\n");
  printf(OTHER_STUFF);
  printf("\n");
  fflush(stdout);
  Dialogue("Hit return", 0);

  modify_print_loop(0);

  return 0;
}
