/* GB_enroll.c -- GB file interace for GB race enrollment program.
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 */
#include <signal.h>
#include "GB_copyright.h"

#define EXTERN
#include "vars.h"
#include "power.h"
#include "ships.h"
#include "shipdata.h"
#include "races.h"
#include "buffers.h"

#include "racegen.h"
#include "config.h"

extern int errno;
racetype *Race;

#define START_RES 100
#define START_MESO_RES_DIFF 40
#define START_FUEL 100
#define START_DES  100

int planet_translate[N_HOME_PLANET_TYPES] =  {0, 6, 7, 5, 2, 3, 4} ;



#ifdef __STDC__
void srandom(int seed) ;
long random(void) ;

int getpid(void) ;

void modify_print_loop(int level) ;


 
#else
#define const
#endif

void notify(who, gov, msg)
     int who, gov;
char *msg;
{
  /* this is a dummy routine */
  }

void warn(who, gov, msg)
     int who, gov;
char *msg;
{
  /* this is a dummy routine */
  }

void push_message(what, who, msg)
     int what, who;
char *msg;
{
  /* this is a dummy routine */
  }


void init_enroll()
{
  srandom(getpid()) ;
  }


/* 
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race()
{
  int mask, x, y, star, pnum, i, ppref, Playernum ;
  int last_star_left, indirect[NUMSTARS] ;
  sectortype *sect;
  planettype *planet;
  startype *star_arena;
/*
  if (race.status == STATUS_ENROLLED) {
    sprintf(race.rejection, "This race has already been enrolled!\n") ;
    return 1 ;
    }
*/
  open_data_files();	
  Playernum = Numraces() + 1 ;
  if ((Playernum == 1) && (race.priv_type != P_GOD)) {
    close_data_files() ;
    sprintf(race.rejection,
	   "The first race enrolled must have God privileges.\n") ;
    return 1 ;
    }
  if (Playernum >= MAXPLAYERS) {
    close_data_files() ;
    sprintf(race.rejection,
	   "There are already %d players; No more allowed.\n", MAXPLAYERS-1);
    race.status = STATUS_UNENROLLABLE ;
    return 1 ;
    }

  getsdata(&Sdata);
  star_arena = (startype *)malloc(Sdata.numstars * sizeof(startype));
  for (star=0; star<Sdata.numstars; star++) {
    Stars[star] = &star_arena[star];
    getstar(&(Stars[star]),star);
    }

  printf("Looking for %s..", planet_print_name[race.home_planet_type]);
  fflush(stdout) ;

  ppref = planet_translate[race.home_planet_type] ;
  for (i=0; i < Sdata.numstars; i++)
    indirect[i] = i ;
  last_star_left = Sdata.numstars - 1 ;
  while (last_star_left >= 0) {
    i = int_rand(0, last_star_left) ;
    star = indirect[i] ;

    printf(".", star) ;
    fflush(stdout) ;
    /* 
     * Skip over inhabited stars and stars with few planets. */
    if ((Stars[star]->numplanets < 2) ||
	Stars[star]->inhabited[0] || Stars[star]->inhabited[1]) {
      }
    else {
      /* look for uninhabited planets */
      for (pnum = 0; pnum < Stars[star]->numplanets; pnum++) {
	getplanet(&planet, star, pnum) ;
	if ((planet->type == ppref) &&
	    (planet->conditions[RTEMP] >= -200) && 
	    (planet->conditions[RTEMP] <= 100))
	  goto found_planet ;
	free(planet);
	}
      }
    /*
     * Since we are here, this star didn't work out: */
    indirect[i] = indirect[last_star_left--] ;
    }

  /*
   * If we get here, then we did not find any good planet. */
  printf(" failed!\n") ;
  sprintf(race.rejection,
	  "Didn't find any free %s; choose another home planet type.\n",
	  planet_print_name[race.home_planet_type]);
  close_data_files();
  race.status = STATUS_UNENROLLABLE ;
  return 1 ;

 found_planet:
  printf(" found!\n") ;
  Race = Malloc(racetype);
  Bzero(*Race);

  Race->Playernum = Playernum ;
  Race->God   = (race.priv_type == P_GOD) ;
  Race->Guest = (race.priv_type == P_GUEST) ;
  strcpy(Race->name, race.name) ;
  strcpy(Race->password, race.password) ;

  strcpy(Race->governor[0].password, "0");
  Race->governor[0].homelevel = Race->governor[0].deflevel = LEVEL_PLAN;
  Race->governor[0].homesystem = Race->governor[0].defsystem = star;
  Race->governor[0].homeplanetnum = Race->governor[0].defplanetnum = pnum;
  /* display options */
  Race->governor[0].toggle.highlight = Playernum;
  Race->governor[0].toggle.inverse = 1;
  Race->governor[0].toggle.color = 0;
  Race->governor[0].active = 1;

  for (i=0; i<=OTHER; i++)
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

  for (i=0; i<MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i==Playernum) || (Playernum==1) || Race->God)
      Race->translate[i-1] = 100;  /* you can talk to own race */
    else
      Race->translate[i-1] = 1;
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
  Race->absorb        = race.attr[ABSORB] ;
  Race->collective_iq = race.attr[COL_IQ] ;
  Race->Metamorph     = (race.race_type == R_METAMORPH) ;
  Race->pods          = race.attr[PODS] ;
  
  Race->fighters      = race.attr[FIGHT] ;
  if (race.attr[COL_IQ] == 1.0)
    Race->IQ_limit    = race.attr[A_IQ] ;
  else
    Race->IQ          = race.attr[A_IQ] ;
  Race->number_sexes  = race.attr[SEXES] ;

  Race->fertilize     = race.attr[FERT] * 100;

  Race->adventurism   = race.attr[ADVENT] ;
  Race->birthrate     = race.attr[BIRTH] ;
  Race->mass          = race.attr[MASS] ;
  Race->metabolism    = race.attr[METAB] ;

  /* 
   * Assign sector compats and determine a primary sector type. */
  for (i = FIRST_SECTOR_TYPE; i<=LAST_SECTOR_TYPE; i++) {
    Race->likes[i] = race.compat[i] / 100.0 ;
    if ((100 == race.compat[i]) &&
	(1.0 == planet_compat_cov[race.home_planet_type][i]))
      Race->likesbest = i ;
    }

  /*
   * Find sector to build capital on, and populate it: */
  getsmap(Smap,planet);
  PermuteSects(planet);
  Getxysect(planet, 0, 0, 1);
  while (i = Getxysect(planet, &x, &y, 0))
    if (Sector(*planet, x, y).condition == Race->likesbest)
      break ;
  if (! i)
    x = y = 0 ;
  sect = &Sector(*planet, x, y) ;
  sect->owner = Playernum ;
  sect->race = Playernum;
  sect->popn = planet->popn = Race->number_sexes ;
  sect->fert = 100 ;
  sect->eff = 10 ;
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
  s.land_x = x ;
  s.land_y = y ;
  
  s.speed = 0;
  s.owner = Playernum;
  s.race = Playernum;
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
  s.mass = s.base_mass + Shipdata[s.type][ABIL_MAXCREW]*Race->mass;
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
  s.damage = 0;  /*Shipdata[s.type][ABIL_DAMAGE];*/
  /* (first capital is 100% efficient */
  s.retaliate = 0;
  
  s.ships = 0;
  
  s.on = 1;
  
  s.name[0] = '\0';
  s.number = shipno;
  putship(&s);
  }


  planet->info[Playernum-1].numsectsowned = 1;
  planet->explored = 0;
  planet->info[Playernum-1].explored = 1;
  /*planet->info[Playernum-1].autorep = 1;*/

  planet->maxpopn = maxsupport(Race, sect, 100.0,0)
      * planet->Maxx * planet->Maxy / 2;
  /* (approximate) */

#ifdef STARTING_INVENTORY

	if (Race->Metamorph) 
		planet->info[Playernum-1].resource += (START_RES - START_MESO_RES_DIFF);
	else
		planet->info[Playernum-1].resource += START_RES;
	
	planet->info[Playernum-1].fuel += START_FUEL;
	planet->info[Playernum-1].destruct += START_DES;
	
#endif

  putrace(Race);
  putsector(sect, planet, x, y) ;
  putplanet(planet,star,pnum);

  /* make star explored and stuff */
  getstar(&Stars[star],star);
  setbit(Stars[star]->explored,Playernum);
  setbit(Stars[star]->inhabited,Playernum);
  Stars[star]->AP[Playernum-1] = 5;
  putstar(Stars[star],star);
  close_data_files();

  sigsetmask(mask);

  printf("Player %d (%s) created on sector %d,%d on %s/%s.\n", Playernum, 
	 race.name, x, y, Stars[star]->name, Stars[star]->pnames[pnum]);
  race.status = STATUS_ENROLLED ;
  return 0 ;
  }
loginfo(){}
