/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * maxsupport() -- return how many people one sector can support
 * compatibility() -- return how much race is compatible with planet
 * gravity() -- return gravity for planet
 * prin_ship_orbits() -- prints place ship orbits
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "power.h"
#include "races.h"
#include "ships.h"
#include <math.h>

int maxsupport(racetype *, sectortype *, double, int);
double compatibility(planettype *, racetype *);
double gravity(planettype *);
char *prin_ship_orbits(shiptype *);
#include "files_shl.p"

int maxsupport(reg racetype *r, reg sectortype *s, reg double c, reg int toxic)
{
    int val;
    double a, b;

    if(!r->likes[s->condition])	
	return 0.0;
    a = ((double)s->eff+1.0)*(double)s->fert;
    b = (.01 * c);

    val = (int)( a * b * .01 * (100.0 - (double)toxic) );

    return val;
}

double compatibility(reg planettype *planet, reg racetype *race)
{
    reg int i,add;
    double sum, atmosphere=1.0;
    
    /* make an adjustment for planetary temperature */
    add = 0.1*((double)planet->conditions[TEMP]-race->conditions[TEMP]);
    sum = 1.0 -(double)abs(add)/100.0;

    /* step through and report compatibility of each planetary gas */
    for (i=TEMP+1; i<=OTHER; i++) {
	add = (double)planet->conditions[i] - race->conditions[i];
	atmosphere *= 1.0 - (double)abs(add)/100.0;
    }
    sum *= atmosphere;
    sum *= 100.0 - planet->conditions[TOXIC];

    if(sum < 0.0) return 0.0;
    return (sum);
}

double gravity(planettype *p)
{
    return (double)(p->Maxx) * (double)(p->Maxy) * GRAV_FACTOR;
}

char Dispshiporbits_buf[PLACENAMESIZE+13];

char *prin_ship_orbits(shiptype *s)
{
    shiptype	*mothership;
    char	*motherorbits;

    switch (s->whatorbits) {
      case LEVEL_UNIV:
	sprintf(Dispshiporbits_buf,"/(%.0f,%.0f)",s->xpos,s->ypos);
	break;
      case LEVEL_STAR:
	sprintf(Dispshiporbits_buf,"/%s", Stars[s->storbits]->name);
	break;
      case LEVEL_PLAN:
	sprintf(Dispshiporbits_buf,"/%s/%s", 
		Stars[s->storbits]->name,
		Stars[s->storbits]->pnames[s->pnumorbits]);
	break;
      case LEVEL_SHIP:
	if (getship(&mothership, s->destshipno)) {
	    motherorbits = prin_ship_orbits(mothership);
	    strcpy(Dispshiporbits_buf, motherorbits);
	    free(mothership);
	} else
	    strcpy(Dispshiporbits_buf, "/");
	break;
      default:
	break;
    }
    return Dispshiporbits_buf;
}
