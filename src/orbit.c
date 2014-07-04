/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  orbit.c -- display orbits of planets (graphic representation)
 *
 * OPTIONS
 *  -p : If this option is set, ``orbit'' will not display planet names.
 *
 *  -S : Do not display star names.
 *
 *  -s : Do not display ships.
 *
 *  -(number) : Do not display that #'d ship or planet (in case it obstructs
 * 		the view of another object)
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
#include <curses.h>
#include <stdio.h>
#include <string.h>
extern char Shipltrs[];
double Lastx, Lasty, Zoom;
int SCALE = 100;

racetype *Race;

char Psymbol[] = {'@', 'o', 'O', '#', '~', '.', ')', '-'};
char *Planet_types[] = {"Class M", "Asteroid", "Airless", "Iceball",
			    "Jovian", "Waterball", "Forest", "Desert"};

void orbit(int, int, int);
void DispStar(int, int, int, startype *, int, int, char *);
void DispPlanet(int, int, int, planettype *, char *, int, racetype *, char *);
void DispShip(int, int, placetype *, shiptype *, planettype *, int, char *);
#include "getplace.p"
#include "GB_server.p"
#include "files_shl.p"
#include "shlmisc.p"
#include "fire.p"
#include "max.p"

void orbit(int Playernum, int Governor, int APcount)
{
register int sh,i,iq;
int DontDispNum= -1, flag;
planettype *p;
shiptype *s;
placetype where;
int DontDispPlanets, DontDispShips, DontDispStars;
char output[100000];

DontDispPlanets = DontDispShips = DontDispStars = 0;

/* find options, set flags accordingly */
for (flag=1; flag<=argn-1; flag++)
    if (*args[flag]=='-') {
	for (i=1; args[flag][i]!='\0'; i++)
	    switch (args[flag][i]) {
	      case 's': DontDispShips = 1;
		break;
	      case 'S': DontDispStars = 1;
		break;
	      case 'p': DontDispPlanets = 1;
		break;
	      default:
		if (sscanf(args[flag]+1,"%d",&DontDispNum)!=1) {
		    sprintf(buf, "Bad number %s.\n", args[flag]+1);
		    notify(Playernum, Governor, buf);
		    DontDispNum = -1;
		}
		if (DontDispNum)
		    DontDispNum--;	/* make a '1' into a '0' */
		break;
	    }
    }

if (argn==1) {
    where = Getplace(Playernum, Governor, ":", 0);
    i = (Dir[Playernum-1][Governor].level==LEVEL_UNIV); 
    Lastx = Dir[Playernum-1][Governor].lastx[i];
    Lasty = Dir[Playernum-1][Governor].lasty[i];
    Zoom = Dir[Playernum-1][Governor].zoom[i];
} else {
    where = Getplace(Playernum, Governor, args[argn-1], 0);
    Lastx = Lasty = 0.0;
    Zoom = 1.1;
}

if (where.err) {
    notify(Playernum, Governor, "orbit: error in args.\n");
    return;
}

/* orbit type of map */
sprintf(output, "#");

Race = races[Playernum-1];

switch (where.level) {
  case LEVEL_UNIV:
    for (i=0; i<Sdata.numstars; i++)
	if (DontDispNum!=i) {
	    DispStar(Playernum, Governor, LEVEL_UNIV, Stars[i], DontDispStars,
		     (int)Race->God, buf);
	    strcat(output, buf);
	}
    if (!DontDispShips) {
	sh = Sdata.ships;
	while (sh) {
	    (void)getship(&s, sh);
	    if (DontDispNum != sh) {
		DispShip(Playernum, Governor, &where, s, NULL,
			 (int)Race->God, buf);
		strcat(output, buf);
	    }
	    sh = s->nextship;
	    free(s);
	}
    }
    break;
  case LEVEL_STAR:
    DispStar(Playernum, Governor, LEVEL_STAR, Stars[where.snum],
	     DontDispStars, (int)Race->God, buf);
    strcat(output, buf);

    for (i=0; i<Stars[where.snum]->numplanets; i++)
	if (DontDispNum!=i) {
	    getplanet(&p,(int)where.snum,i);
	    DispPlanet(Playernum, Governor, LEVEL_STAR, p,
		       Stars[where.snum]->pnames[i],DontDispPlanets, Race, buf);
	    strcat(output, buf);
	    free(p);
	}
    /* check to see if you have ships at orbiting the star, if so you can
       see enemy ships */
    iq = 0;
    if(Race->God) iq = 1;
    else {
	sh = Stars[where.snum]->ships;
	while (sh && !iq) {
	    (void)getship(&s, sh);
	    if(s->owner == Playernum && Sight(s))
		iq = 1; /* you are there to sight, need a crew */
	    sh = s->nextship;
	    free(s);
	}
    }
    if (!DontDispShips) {
	sh = Stars[where.snum]->ships;
	while (sh) {
	    (void)getship(&s, sh);
	    if (DontDispNum != sh &&
		!(s->owner != Playernum && s->type == STYPE_MINE) ) {
		if((s->owner == Playernum) || (iq == 1)) {
		    DispShip(Playernum, Governor, &where, s, NULL,
			     (int)Race->God, buf);
		    strcat(output, buf);
		}
	    }
	    sh = s->nextship;
	    free(s);
	}
    }
    break;
  case LEVEL_PLAN:
    getplanet(&p,(int)where.snum,(int)where.pnum);
    DispPlanet(Playernum, Governor, LEVEL_PLAN, p,
	       Stars[where.snum]->pnames[where.pnum],
	       DontDispPlanets, Race, buf);
    strcat(output, buf);

/* check to see if you have ships at landed or
   orbiting the planet, if so you can see orbiting enemy ships */
    iq = 0;
    sh = p->ships;
    while (sh && !iq) {
	(void)getship(&s, sh);
	if(s->owner == Playernum && Sight(s))
	    iq = 1; /* you are there to sight, need a crew */
	sh = s->nextship;
	free(s);
    }
/* end check */
    if (!DontDispShips) {
	sh = p->ships;
	while (sh) {
	    (void)getship(&s, sh);
	    if (DontDispNum != sh) {
	     	if(!landed(s)) {
		    if((s->owner == Playernum) || (iq ==1)) {
			DispShip(Playernum, Governor, &where, s, p,
				 (int)Race->God, buf);
			strcat(output, buf);
		    }
		}
	    }
	    sh = s->nextship;
	    free(s);
	}
    }
    free(p);
    break;
  default:
    notify(Playernum, Governor,"Bad scope.\n");
    return;	 
}
strcat(output, "\n");
notify(Playernum, Governor, output);
}

void DispStar(int Playernum, int Governor, int level, startype *star,
	      int DontDispStars, int God, char *string)
{
    int x,y;
    int stand;
    int iq;
    double fac;

    *string = '\0'; 

    if (level==LEVEL_UNIV) {
	fac = 1.0;
	x = (int)(SCALE+((SCALE*(star->xpos-Lastx))/(UNIVSIZE*Zoom)));
	y = (int)(SCALE+((SCALE*(star->ypos-Lasty))/(UNIVSIZE*Zoom)));
    } else if (level==LEVEL_STAR) {
	fac = 1000.0;
	x = (int)(SCALE+(SCALE*(-Lastx))/(SYSTEMSIZE*Zoom));
	y = (int)(SCALE+(SCALE*(-Lasty))/(SYSTEMSIZE*Zoom));
    }
    /*if (star->nova_stage)
      DispArray(x, y, 11,7, Novae[star->nova_stage-1], fac); */
    if (y>=0 && x>=0) {
	iq = 0;
	if(Race->governor[Governor].toggle.color) {
	    stand = (isset(star->explored, Playernum) ? Playernum : 0) + '?';
	    sprintf(temp, "%c %d %d 0 * ", (char)stand, x, y);
	    strcat(string, temp);
	    stand = (isset(star->inhabited, Playernum) ? Playernum : 0) + '?';
	    sprintf(temp, "%c %s;", (char)stand, star->name);
	    strcat(string, temp);
	} else {
	    stand =  (isset(star->explored,Playernum) ? 1 : 0);
	    sprintf(temp, "%d %d %d 0 * ", stand, x, y);
	    strcat(string, temp);
	    stand = (isset(star->inhabited, Playernum) ? 1 : 0);
	    sprintf(temp, "%d %s;", stand, star->name);
	    strcat(string, temp);
	}
    }
}

void DispPlanet(int Playernum, int Governor, int level, planettype *p,
		char *name, int DontDispPlanets, racetype *r, char *string)
{
    int x,y;
    int stand;

    *string = '\0';

    if (level==LEVEL_STAR) {
	y = (int)(SCALE+(SCALE*(p->ypos-Lasty))/(SYSTEMSIZE*Zoom));
	x = (int)(SCALE+(SCALE*(p->xpos-Lastx))/(SYSTEMSIZE*Zoom));
    } else if (level==LEVEL_PLAN) {
	y = (int)(SCALE+(SCALE*(-Lasty))/(PLORBITSIZE*Zoom));
	x = (int)(SCALE+(SCALE*(-Lastx))/(PLORBITSIZE*Zoom));
    }
    if (x>=0 && y>=0) {
	if(r->governor[Governor].toggle.color) {
	    stand = (p->info[Playernum-1].explored ? Playernum : 0) + '?';
	    sprintf(temp, "%c %d %d 0 %c ", (char)stand, x, y,
		    (stand > '0' ? Psymbol[p->type] : '?'));
	    strcat(string, temp);
	    stand = (p->info[Playernum-1].numsectsowned ? Playernum : 0) + '?';
	    sprintf(temp, "%c %s", (char)stand, name);
	    strcat(string, temp);
	} else {
	    stand = p->info[Playernum-1].explored ? 1 : 0;
	    sprintf(temp, "%d %d %d 0 %c ", stand, x, y,
		    (stand?Psymbol[p->type]:'?'));
	    strcat(string, temp);
	    stand = p->info[Playernum-1].numsectsowned ? 1 : 0;
	    sprintf(temp, "%d %s", stand, name);
	    strcat(string, temp);
	}
	if(r->governor[Governor].toggle.compat && p->info[Playernum-1].explored) {
	    sprintf(temp,"(%d)", (int)compatibility(p,r));
	    strcat(string, temp);
	}
	strcat(string, ";");
    }
}

void DispShip(int Playernum, int Governor, placetype *where, shiptype *ship,
	      planettype *pl , int God, char *string)
{
 int x,y,wm;
 int stand;
 shiptype *aship;
 planettype *apl;
 double xt,yt,slope;

 if (!ship->alive)
     return;

 *string = '\0';

 switch (where->level) {
   case LEVEL_PLAN:
     x = (int)(SCALE + (SCALE*(ship->xpos-(Stars[where->snum]->xpos+pl->xpos)
			       - Lastx))/(PLORBITSIZE*Zoom));
     y = (int)(SCALE + (SCALE*(ship->ypos-(Stars[where->snum]->ypos+pl->ypos)
			       - Lasty))/(PLORBITSIZE*Zoom));
     break;
   case LEVEL_STAR:
     x = (int)(SCALE + (SCALE*(ship->xpos-Stars[where->snum]->xpos - Lastx))
	       /(SYSTEMSIZE*Zoom));
     y = (int)(SCALE + (SCALE*(ship->ypos-Stars[where->snum]->ypos - Lasty))
	       /(SYSTEMSIZE*Zoom));
     break;
   case LEVEL_UNIV:
     x = (int)(SCALE + (SCALE*(ship->xpos-Lastx))/(UNIVSIZE*Zoom));
     y = (int)(SCALE + (SCALE*(ship->ypos-Lasty))/(UNIVSIZE*Zoom));
     break;
   default:
     notify(Playernum, Governor, "WHOA! error in DispShip.\n");
     return;
 }

 switch (ship->type) {
   case STYPE_MIRROR:
     if (ship->special.aimed_at.level==LEVEL_STAR) {
	 xt = Stars[ship->special.aimed_at.snum]->xpos;
	 yt = Stars[ship->special.aimed_at.snum]->ypos;
     } else if (ship->special.aimed_at.level==LEVEL_PLAN) {
	 if (where->level==LEVEL_PLAN && 
	     ship->special.aimed_at.pnum == where->pnum) {  
	     /* same planet */
	     xt = Stars[ship->special.aimed_at.snum]->xpos + pl->xpos;
	     yt = Stars[ship->special.aimed_at.snum]->ypos + pl->ypos;
	 } else {	/* different planet */
	     getplanet(&apl,(int)where->snum,(int)where->pnum);
	     xt = Stars[ship->special.aimed_at.snum]->xpos + apl->xpos;
	     yt = Stars[ship->special.aimed_at.snum]->ypos + apl->ypos;
	     free(apl);
	 }
     } else if (ship->special.aimed_at.level==LEVEL_SHIP) {
	 if (getship(&aship,(int)ship->special.aimed_at.shipno)) {
	     xt = aship->xpos;
	     yt = aship->ypos;
	     free(aship);
	 } else
	     xt = yt = 0.0;
     } else
	 xt = yt = 0.0;
     wm=0;

     if(xt == ship->xpos) {
	 if(yt > ship->ypos)wm=4;
	 else wm=0;
     } else {
	 slope = (yt - ship->ypos) / (xt - ship->xpos);
	 if(yt == ship->ypos) {
	     if(xt > ship->xpos)wm=2;
	     else wm=6;
	 } else if(yt > ship->ypos) {
	     if(slope < -2.414)wm=4;
	     if(slope > -2.414)wm=5;
	     if(slope > -0.414)wm=6;
	     if(slope >  0.000)wm=2;
	     if(slope >  0.414)wm=3;
	     if(slope >  2.414)wm=4;
	 } else if(yt < ship->ypos) {
	     if(slope < -2.414)wm=0;
	     if(slope > -2.414)wm=1;
	     if(slope > -0.414)wm=2;
	     if(slope >  0.000)wm=6;
	     if(slope >  0.414)wm=7;
	     if(slope >  2.414)wm=0;
	 }
     }

     /* (magnification) */
     if (x>=0 && y>=0) {
	 if(Race->governor[Governor].toggle.color) {
	     sprintf(string, "%c %d %d %d %c %c %d;",
		     (char)(ship->owner+'?'), x, y, wm,
		     Shipltrs[ship->type], (char)(ship->owner+'?'),
		     ship->number);
	 } else {
	     stand = (ship->owner==Race->governor[Governor].toggle.highlight);
	     sprintf(string, "%d %d %d %d %c %d %d;",
		     stand, x, y, wm, Shipltrs[ship->type], stand,
		     ship->number);
	 }
     }
     break;

   case OTYPE_CANIST:
   case OTYPE_GREEN:
     break;

   default:
     /* other ships can only be seen when in system */
     wm=0;
     if (ship->whatorbits!=LEVEL_UNIV || ((ship->owner == Playernum) || God))
	 if (x>=0 && y>=0) {
	     if(Race->governor[Governor].toggle.color) {
		 sprintf(string, "%c %d %d %d %c %c %d;",
			 (char)(ship->owner+'?'), x, y, wm,
			 Shipltrs[ship->type], (char)(ship->owner+'?'),
			 ship->number);
	     } else {
		stand =
		    (ship->owner==Race->governor[Governor].toggle.highlight);
		sprintf(string, "%d %d %d %d %c %d %d;",
			stand, x, y, wm, Shipltrs[ship->type], stand,
			ship->number);
	    }
	 }
     break;
 }
}


