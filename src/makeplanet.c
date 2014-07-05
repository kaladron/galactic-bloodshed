/* makeplanet.c -- makes one planet.
 *
 * #ident  "@(#)makeplanet.c	1.3 2/17/93 "
 *
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * Modified Feb 13, 1992 by Shawn Fox : skf5055@tamsun.tamu.edu
 */

#include "GB_copyright.h"

#define MAP_ISLANDS 3		/* # of beginning islands for makeuniv */
#define MAP_MOUNT_PERCENT 0.22	/* percentage of mountain areas */
#define MAP_DESERT_PERCENT 0.10	/* percent of desert areas */
#define MAP_GASGIANT_BANDMIN 20  /* min size gasgiants that have bands */
#define LANDPERCENTAGE int_rand(20,70)/100
#define POLEFUDGE 10

#define EXTERN extern
#include "vars.h"
#include <string.h>
#include <math.h>

extern double double_rand(void);
extern int int_rand(int, int);
extern int round_rand(double);
extern int rposneg(void);


/*             @   o   O   #   ~   .   (   -    */
int xmin[] = {15,  2,  4,  4, 26, 12, 12, 12} ;
int xmax[] = {23,  4,  8,  8, 32, 20, 20, 20} ;

/*
 * Fmin, Fmax, rmin, rmax are now all based on the sector type as well 
 * as the planet type.
 */

/*                 .      *     ^     ~     #     (     -             */
int x_chance[]= {  5,    15,   10,    4,    5,    7,    6};
int Fmin[][8] ={{ 25,    20,   10,    0,   20,   45,    5},  /*   @   */
                {  0,     1,    2,    0,    0,    0,    1},  /*   o   */
                {  0,     3,    2,    0,    0,    0,    2},  /*   O   */
                {  0,     0,    8,    0,   25,    0,    0},  /*   #   */
                {  0,     0,    0,   35,    0,    0,    0},  /*   ~   */
                { 30,     0,    0,    0,   20,    0,    0},  /*   .   */
                { 30,    25,   20,    0,   30,   60,   15},  /*   (   */
                {  0,     5,    2,    0,    0,    0,    2}}; /*   -   */

/*                 .      *     ^     ~     #     (     -             */
int Fmax[][8] ={{ 40,    35,   20,    0,   40,   65,   15},  /*   @   */
                {  0,     2,    3,    0,    0,    0,    2},  /*   o   */
                {  0,     5,    4,    0,    0,    0,    3},  /*   O   */
                {  0,     0,   15,    0,   35,    0,    0},  /*   #   */
                {  0,     0,    0,   55,    0,    0,    0},  /*   ~   */
                { 50,     0,    0,    0,   40,    0,    0},  /*   .   */
                { 60,    45,   30,    0,   50,   90,   25},  /*   (   */
                {  0,    10,    6,    0,    0,    0,    8}}; /*   -   */
 
/*                 .      *     ^     ~     #     (     -             */
int rmin[][8] ={{200,   225,  300,    0,  200,    0,  250},  /*   @   */
                {  0,   250,  350,    0,    0,    0,  300},  /*   o   */
                {  0,   225,  275,    0,    0,    0,  250},  /*   O   */
                {  0,     0,  250,    0,  225,    0,    0},  /*   #   */
                {  0,     0,    0,   30,    0,    0,    0},  /*   ~   */
                {175,     0,    0,    0,  200,    0,    0},  /*   .   */
                {150,     0,    0,    0,  150,  150,    0},  /*   (   */
                {  0,   200,  300,    0,    0,    0,  250}}; /*   -   */

/*                 .      *     ^     ~     #     (     -             */
int rmax[][8] ={{250,   325,  400,    0,  250,    0,  300},  /*   @   */
                {  0,   300,  600,    0,    0,    0,  400},  /*   o   */
                {  0,   300,  500,    0,    0,    0,  300},  /*   O   */
                {  0,     0,  350,    0,  300,    0,    0},  /*   #   */
                {  0,     0,    0,   60,    0,    0,    0},  /*   ~   */
                {225,     0,    0,    0,  250,    0,    0},  /*   .   */
                {250,     0,    0,    0,  250,  200,    0},  /*   (   */
                {  0,   200,  500,    0,    0,    0,  350}}; /*   -   */

/*  The starting conditions of the sectors given a planet types */
/*              @      o     O    #    ~    .       (       _  */
int cond[] = {SEA, MOUNT, LAND, ICE, GAS, SEA, FOREST, DESERT};

extern int    Temperature(double, int);

double DistmapSq(int, int, int, int);
void   MakeEarthAtmosphere(planettype *, int);
int    neighbors(planettype *, int, int, int);
int    SectorTemp(planettype *, int, int);
void   Makesurface(planettype *);
short  SectTemp(planettype *, int);
planettype Makeplanet(double, short, int);
void   seed (planettype *, int, int);
void   grow (planettype *, int, int, int);

planettype Makeplanet(double dist, short stemp, int type)
{
    reg int x, y;
    sectortype *s;
    planettype planet;
    int i, atmos, total_sects;
    char c, t;
    double f;
    
    Bzero(planet);
    bzero((char *)Smap, sizeof(Smap));
    planet.type = type;
    planet.expltimer = 5;
    planet.conditions[TEMP] = planet.conditions[RTEMP] = Temperature(dist, stemp);
  
    planet.Maxx = int_rand(xmin[type], xmax[type]) ;
    f = (double)planet.Maxx / RATIOXY;
    planet.Maxy = round_rand(f) + 1;
    if(!(planet.Maxy % 2))
	planet.Maxy++;		/* make odd number of latitude bands */

    if (type == TYPE_ASTEROID)
	planet.Maxy = int_rand(1, 3) ;   /* Asteroids have funny shapes. */

    t = c = cond[type];

    for (y=0; y<planet.Maxy; y++) {
	for (x=0; x<planet.Maxx; x++) {
	    s = &Sector(planet,x,y);
	    s->type = s->condition = t;
	}
    }

    total_sects = (planet.Maxy-1)*(planet.Maxx-1);
  
    switch (type) {
      case TYPE_GASGIANT:		/* gas giant planet */
	/* either lots of meth or not too much */
	if (int_rand(0, 1)) {	/* methane planet */
	    atmos = 100 - (planet.conditions[METHANE] = int_rand(70, 80)); 
	    atmos -= planet.conditions[HYDROGEN] = int_rand(1, atmos/2);
	    atmos -= planet.conditions[HELIUM] = 1;
	    atmos -= planet.conditions[OXYGEN] = 0;
	    atmos -= planet.conditions[CO2] = 1;
	    atmos -= planet.conditions[NITROGEN] = int_rand(1, atmos/2);
	    atmos -= planet.conditions[SULFUR] = 0;
	    planet.conditions[OTHER] = atmos;
	} else {
	    atmos = 100 - (planet.conditions[HYDROGEN] = int_rand(30,75));
	    atmos -= planet.conditions[HELIUM] = int_rand(20,atmos/2);
	    atmos -= planet.conditions[METHANE] = random()&01;
	    atmos -= planet.conditions[OXYGEN] = 0;
	    atmos -= planet.conditions[CO2] = random()&01;
	    atmos -= planet.conditions[NITROGEN] = int_rand(1,atmos/2);
	    atmos -= planet.conditions[SULFUR] = 0;
	    planet.conditions[OTHER] = atmos;
	}
	break;
      case TYPE_MARS:
	planet.conditions[HYDROGEN] = 0;
	planet.conditions[HELIUM] = 0;
	planet.conditions[METHANE] = 0;
	planet.conditions[OXYGEN] = 0;
	if (random()&01) {    /* some have an atmosphere, some don't */
	    atmos = 100 - (planet.conditions[CO2] = int_rand(30,45));
	    atmos -= planet.conditions[NITROGEN] = int_rand(10, atmos/2);
	    atmos -= planet.conditions[SULFUR] = 
		(random()&01) ? 0 : int_rand(20,atmos/2) ;
	    atmos -= planet.conditions[OTHER] = atmos;
	} else {
	    planet.conditions[CO2] = 0;
	    planet.conditions[NITROGEN] = 0;
	    planet.conditions[SULFUR] = 0;
	    planet.conditions[OTHER] = 0;
	}
        seed (&planet, DESERT, int_rand (1,total_sects));
        seed (&planet, MOUNT, int_rand (1,total_sects));
	break;
      case TYPE_ASTEROID: 		/* asteroid */
	/* no atmosphere */
	for (y=0; y<planet.Maxy; y++)
	    for (x=0; x<planet.Maxx; x++)
		if (!int_rand(0,3)) {
		    s = &Sector(planet, int_rand(1,planet.Maxx), 
				int_rand(1,planet.Maxy));
		    s->type = s->condition = LAND ;
		}
        seed (&planet, DESERT, int_rand (1,total_sects));
	break;
      case TYPE_ICEBALL:		/* ball of ice */
	/* no atmosphere */
	planet.conditions[HYDROGEN] = 0;
	planet.conditions[HELIUM] = 0;
	planet.conditions[METHANE] = 0;
	planet.conditions[OXYGEN] = 0;
	if (planet.Maxx * planet.Maxy > int_rand(0,20)) {
	    atmos = 100 - (planet.conditions[CO2] = int_rand(30,45)) ;
	    atmos -= planet.conditions[NITROGEN] = int_rand(10, atmos/2) ;
	    atmos -= planet.conditions[SULFUR] = 
		(random()&01) ? 0 : int_rand(20, atmos/2) ;
	    atmos -= planet.conditions[OTHER] = atmos ;
	} else {
	    planet.conditions[CO2] = 0;
	    planet.conditions[NITROGEN] = 0;
	    planet.conditions[SULFUR] = 0;
	    planet.conditions[OTHER] = 0;
	}
        seed (&planet, MOUNT, int_rand (1, total_sects/2));
	break;
      case TYPE_EARTH:
	MakeEarthAtmosphere(&planet, 33) ;
        seed (&planet, LAND, int_rand (total_sects/30, total_sects/20));
        grow (&planet, LAND, 1, 1);
        grow (&planet, LAND, 1, 2);
        grow (&planet, LAND, 2, 3);
        grow (&planet, SEA, 1, 4); 
	break;
      case TYPE_FOREST:
	MakeEarthAtmosphere(&planet, 0);
        seed (&planet, SEA, int_rand(total_sects/30, total_sects/20));
        grow (&planet, SEA, 1, 1);
        grow (&planet, SEA, 1, 3);
        grow (&planet, FOREST, 1, 3);
	break;
      case TYPE_WATER:
	MakeEarthAtmosphere(&planet, 25) ;
        break;
      case TYPE_DESERT:
	MakeEarthAtmosphere(&planet, 50) ;
        seed (&planet, MOUNT, int_rand(total_sects/50,total_sects/25));
        grow (&planet, MOUNT, 1, 1);
        grow (&planet, MOUNT, 1, 2);
        seed (&planet, LAND, int_rand(total_sects/50,total_sects/25));
        grow (&planet, LAND, 1, 1);
        grow (&planet, LAND, 1, 3);
        grow (&planet, DESERT, 1, 3);
	break;
    }
    Makesurface(&planet);  /* determine surface geology based on environment */
    return planet;
}

void MakeEarthAtmosphere(planettype *pptr, int chance)
{
    int atmos = 100 ;
  
    if (int_rand(0,99) > chance) {
	/* oxygen-reducing atmosphere */
	atmos -= pptr->conditions[OXYGEN] = int_rand(10, 25) ;
	atmos -= pptr->conditions[NITROGEN] = int_rand(20, atmos-20);
	atmos -= pptr->conditions[CO2] = int_rand(10, atmos/2);
	atmos -= pptr->conditions[HELIUM] = int_rand(2, atmos/8+1);
	atmos -= pptr->conditions[METHANE] = random()&01;
	atmos -= pptr->conditions[SULFUR] = 0;
	atmos -= pptr->conditions[HYDROGEN] = 0;
	pptr->conditions[OTHER] = atmos;
    } else {
	/* methane atmosphere */
	atmos -= pptr->conditions[METHANE] = int_rand(70, 80) ;
	atmos -= pptr->conditions[HYDROGEN] = int_rand(1, atmos/2);
	atmos -= pptr->conditions[HELIUM] = 1 + (random()&01) ;
	atmos -= pptr->conditions[OXYGEN] = 0;
	atmos -= pptr->conditions[CO2] = 1 + (random()&01) ;
	atmos -= pptr->conditions[SULFUR] = (random()&01) ;
	atmos -= pptr->conditions[NITROGEN] = int_rand(1, atmos/2);
	pptr->conditions[OTHER] = atmos;
    }
}

double DistmapSq(int x, int y, int x2, int y2)
{
#if 0
    return fabs((double)(x-x2)) / RATIOXY + fabs( (double)(y-y2));
#else
    return (0.8*(x-x2)*(x-x2) + (y-y2)*(y-y2)) ;
#endif
}


int SectorTemp(planettype *pptr, int x, int y)
{		
    int p_x, p_xg ;       /* X of the pole, and the ghost pole. */
    int p_y ;             /* Y of the nearest pole. */
    double f, d ;         /* `distance' to pole. */
    static double renorm[] = 
    {0, 1.0/1.0, 2.0/2.0, 4.0/3.0, 6.0/4.0, 9.0/5.0, 12.0/6.0, 16.0/7.0,
	 20.0/8.0, 25.0/9.0, 30.0/10.0, 36.0/11.0, 42.0/12.0, 49.0/13.0} ;
    /*                        @   o   O   #   ~   .   (   -    */
    static int variance[] = {30, 40, 40, 40, 10, 25, 30, 30} ;
    

  /* I use pptr->sectormappos to calculate the pole position from.
     This in spite of the fact that the two have nothing to do with each other.
     I did it because (a) I don't want the pole to move, and sectormappos will
     also not change, and (b) sectormappos will not show up to the player in
     any other fashion. */
    p_x = pptr->sectormappos % pptr->Maxx ;
    if (y < (pptr->Maxy / 2.0))
	p_y = -1 ;
    else {
	p_y = pptr->Maxy ;
	p_x = p_x + pptr->Maxx / 2.0 ;
	if (p_x >= pptr->Maxx)
	    p_x -= pptr->Maxx ;
    }
    if (p_x <= (pptr->Maxy / 2))
	p_xg = p_x + pptr->Maxy ;
    else
	p_xg = p_x - pptr->Maxy ;
    d = (y-p_y) * (y-p_y) ;

    f = (x-p_x+0.2) / pptr->Maxx ;
    if (f < 0.0)
	f = -f ;
    if (f > 0.5)
	f = 1.0 - f ;
    d = sqrt(d + f - 0.5) ;
    return (pptr->conditions[RTEMP] + 
	    variance[pptr->type] * (d - renorm[pptr->Maxy])) ;
}

/* 
    Returns # of neighbors of a given designation that a sector has. 
*/

int neighbors (planettype *p, int x, int y, int type)
{
    int l = x - 1 ;
    int r = x + 1 ;    /* Left and right columns. */
    int n = 0 ;        /* Number of neighbors so far. */

    if (x == 0)
	l = p->Maxx - 1 ;
    else if (r == p->Maxx)
	r = 0 ;
    if (y > 0)
	n += (Sector(*p,x,y-1).type == type) +
             (Sector(*p,l,y-1).type == type) +
             (Sector(*p,r,y-1).type == type);

    n += (Sector(*p,l,y).type == type) +
	 (Sector(*p,r,y).type == type) ;

    if (y < p->Maxy-1)
	n += (Sector(*p,x,y+1).type == type) +
             (Sector(*p,l,y+1).type == type) +
             (Sector(*p,r,y+1).type == type);

    return (n); 
}

/*
 * Randomly places n sectors of designation type on a planet.
 */

void seed (planettype *p, int type, int n)
{
   int x,y;
   sectortype *s;

   while (n-- > 0)
   {
      x = int_rand (0, p->Maxx-1);
      y = int_rand (0, p->Maxy-1);
      s = &Sector(*p,x,y);
      s->type = s->condition = type;
   }
}

/*
 * Spreat out a sector of a certain type over the planet.  Rate is the number
 * of adjacent sectors of the same type that must be found for the setor to
 * become type.
 */

void grow (planettype *p, int type, int n, int rate)
{
   int x,y;
   sectortype *s;
   sectortype Smap2[(MAX_X+1)*(MAX_Y+1)+1];

   while (n-- > 0) {
      memcpy (Smap2, Smap, sizeof (Smap));
      for (x=0;x<p->Maxx;x++) {
         for (y=0;y<p->Maxy;y++) {
            if (neighbors (p,x,y,type) >= rate) {
               s = &Smap2[x+y*p->Maxx+1];
               s->condition = s->type = type;
            }
         }
      }
      memcpy (Smap, Smap2, sizeof (Smap));
   }
}

void Makesurface(planettype *p)
{
    reg int r,x,y;
    reg int x2,y2, xx, temp;
    reg sectortype *s;
    double rr;

    for(x=0; x<p->Maxx; x++) {
	for(y=0;y<p->Maxy; y++) {
	    s = &Sector(*p, x, y);
	    temp = SectTemp(p, y);
	    switch(s->type) {
	      case SEA:
		if(success(-temp) && ((y == 0) || (y == p->Maxy-1)))
		    s->condition = ICE;
		break;
	      case LAND:
                if(p->type == TYPE_EARTH) {
                   if (success(-temp) && (y == 0 || y == p->Maxy-1))
                      s->condition = ICE;
                }
		break;
              case FOREST:
                if (p->type == TYPE_FOREST) {
                   if (success(-temp) && (y == 0 || y == p->Maxy-1))
                      s->condition = ICE;
                }
	    }
            s->type = s->condition;
            s->resource = int_rand (rmin[p->type][s->type],
                                    rmax[p->type][s->type]);
            s->fert = int_rand (Fmin[p->type][s->type],
                                Fmax[p->type][s->type]);
	    if(int_rand(0, 1000) < x_chance[s->type])
		s->crystals=int_rand(4,8);
	    else
		s->crystals = 0;
	}
    }
}

#define TFAC 10

short SectTemp(planettype *p, int y)
{
  register int i, dy, mid, temp;

  temp = p->conditions[TEMP];
  mid = (p->Maxy+1)/2-1;
  dy = abs(y-mid);

  temp -= TFAC*dy*dy;
  return temp;
/*  return(p->conditions[TEMP]); */
}

