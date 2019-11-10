// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef RACEGEN_H
#define RACEGEN_H

#define GBVERSION "1.7.3"
#ifdef VERSION
#undef VERSION
#endif
#define VERSION ""

/**************
 * This #define is used to compile the code in this program needed for the
 * enroll program.  Unless you are a game-god, you will never need to use it.
 */
#ifdef ENROLL
#define IS_PLAYER 0
#else
#define IS_PLAYER 1
#endif

/**************
 * System dependencies.  These will likely not change much.
 */
#define MAILER "/usr/lib/sendmail"
#define SAVETO "racegen.save"
#define TMP "/tmp/racegen.save"

/**************
 * Other stuff.
 */
#define START_RECORD_STRING "<************"
#define END_RECORD_STRING "************>"

int Dialogue(const char *, ...);

/**************
 * Attributes, attribute names, and parameters for attribute costs.
 */
#define FIRST_ATTRIBUTE 0
#define ADVENT FIRST_ATTRIBUTE
#define ABSORB (ADVENT + 1)
#define BIRTH (ABSORB + 1)
#define COL_IQ (BIRTH + 1)
#define FERT (COL_IQ + 1)
#define A_IQ (FERT + 1)
#define FIGHT (A_IQ + 1)
#define PODS (FIGHT + 1)
#define MASS (PODS + 1)
#define SEXES (MASS + 1)
#define METAB (SEXES + 1)
#define LAST_ATTRIBUTE (METAB)
#define N_ATTRIBUTES (LAST_ATTRIBUTE + 1)

typedef struct {
  int number;
  char print_name[16];
  double e_factor, e_fudge, e_hinge, l_factor, l_fudge;
  double minimum, init, maximum;
  double cov[N_ATTRIBUTES];
  int is_integral;
} attribute;

#define ATTR_RANGE(a) (attr[a].maximum - attr[a].minimum)

/* The formula for determining the price of any particular attribute
 * is as follows:
 *   exp( (e_fudge * (attribute - e_hinge) )) * e_factor
 *     + attribute * l_factor + l_fudge
 * This allows great flexibility in generating functions for
 * attribute costs.      */

/* Increasing an attribute's e_factor will raise the cost of the attribute
 * everywhere (since exp(x)>0 forall x); however, it raises the cost quite
 * disproportionately.  If a correponding decrease in l_fudge is made, this
 * will have little effect on the cost to buy an attribute below the hinge
 * point, but will have a strong effect on the cost above the hinge.     */

/* Increasing an attribute's e_fudge will have the effect of driving the cost
 * of attributes below the hinge down (slightly), and driving the cost of
 * attributes above the hinge significantly higher.     */

/* An attribute's e_hinge is the point is that the exponential "takes off";
 * that is, the exponential will have a small effect on the cost for an
 * attribute below this value, but will have a much larger impact for
 * those over this value.      */

/* An attribute's l_factor (linear factor) allows you to increase the
 * cost over the whole range of the attribute, in a smoothly increasing (or
 * decreasing) way.      */

/* The l_fudge value is a constant adjustment to the cost of an attribute.
 * It is used to get the init value of an attribute to cost zero.
 * It is set automatically at startup, so don't bother to mess with it.    */

/**************
 * Home planet types, names, and costs.
 */
#define FIRST_HOME_PLANET_TYPE 0
#define H_EARTH FIRST_HOME_PLANET_TYPE
#define H_FOREST (H_EARTH + 1)
#define H_DESERT (H_FOREST + 1)
#define H_WATER (H_DESERT + 1)
#define H_AIRLESS (H_WATER + 1)
#define H_ICEBALL (H_AIRLESS + 1)
#define H_JOVIAN (H_ICEBALL + 1)
#define LAST_HOME_PLANET_TYPE H_JOVIAN
#define N_HOME_PLANET_TYPES (LAST_HOME_PLANET_TYPE + 1)

extern const char *planet_print_name[N_HOME_PLANET_TYPES];
extern const int planet_cost[N_HOME_PLANET_TYPES];

/**************
 * Race types, names, and costs
 */
#define FIRST_RACE_TYPE 0
#define R_NORMAL FIRST_RACE_TYPE
#define R_METAMORPH (R_NORMAL + 1)
#define LAST_RACE_TYPE R_METAMORPH
#define N_RACE_TYPES (LAST_RACE_TYPE + 1)

extern const char *race_print_name[N_RACE_TYPES];
extern const int race_cost[N_RACE_TYPES];

/**************
 * Type of privileges this race will have:
 */
#define FIRST_PRIV_TYPE 0
#define P_GOD (FIRST_PRIV_TYPE)
#define P_GUEST (P_GOD + 1)
#define P_NORMAL (P_GUEST + 1)
#define LAST_PRIV_TYPE (P_NORMAL)
#define N_PRIV_TYPES (LAST_PRIV_TYPE + 1)

extern const char *priv_print_name[N_PRIV_TYPES];

/**************
 * Sector types and names.  Sector costs are hardwired in currently.
 */
#define FIRST_SECTOR_TYPE 0
#define S_WATER FIRST_SECTOR_TYPE
#define S_LAND (S_WATER + 1)
#define S_MOUNTAIN (S_LAND + 1)
#define S_GAS (S_MOUNTAIN + 1)
#define S_ICE (S_GAS + 1)
#define S_FOREST (S_ICE + 1)
#define S_DESERT (S_FOREST + 1)
#define S_PLATED (S_DESERT + 1)
#define LAST_SECTOR_TYPE S_PLATED
#define N_SECTOR_TYPES (LAST_SECTOR_TYPE + 1)

extern const char *sector_print_name[N_SECTOR_TYPES];
extern const int n_sector_types_cost[N_SECTOR_TYPES];

/*
 * The covariance between two sectors is:
 *   actual_cost(a1) = base_cost(a1) *
 *                      (1 + cov[a1][a2] * (a2 - cov[a1][a2].fudge)) ;
 */
extern const double compat_cov[N_SECTOR_TYPES][N_SECTOR_TYPES];
extern const double planet_compat_cov[N_HOME_PLANET_TYPES][N_SECTOR_TYPES];

#define STATUS_ENROLLED -2
#define STATUS_UNENROLLABLE -1
#define STATUS_UNBALANCED 0
#define STATUS_BALANCED 1

/**************
 * Structure for holding information about a race.
 */
struct x {
  char address[64]; /* Person who this is from, or going to. */
  char filename[64];
  char name[64];
  char password[64];
  char rejection[256]; /* Error if this is non-"" */
  char status;

  double attr[N_ATTRIBUTES];
  int race_type;
  int priv_type;
  int home_planet_type;
  int n_sector_types;
  double compat[N_SECTOR_TYPES];
};

/**************
 * Global variables for this program.
 */
extern struct x race_info, cost_info, last;

extern int npoints;
extern int last_npoints;
extern int altered;     /* 1 iff race has been altered since last saved */
extern int changed;     /* 1 iff race has been changed since last printed */
extern int please_quit; /* 1 iff you want to exit ASAP. */

int critique_to_file(FILE *f, int rigorous_checking, int is_player_race);
void print_to_file(FILE *f, int verbose);
int load_from_file(FILE *g);
int cost_of_race();
void modify_print_loop(int level);

#endif  // RACEGEN_H
