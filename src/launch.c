/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  launch.c -- launch or undock a ship (also undock)
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
#include <signal.h>

void launch(int, int, int);
#include "GB_server.p"
#include "shlmisc.p"
#include "fire.p"
#include "files_shl.p"
#include "max.p"
#include "rand.p"
#include "load.p"

void launch(int Playernum, int Governor, int APcount)
{
int sh2;
shiptype *s,*s2;
planettype *p;
int shipno, i, nextshipno;
double fuel;

if(argn < 2) {
    notify(Playernum, Governor, "Launch what?\n");
    return;
}

nextshipno = start_shiplist(Playernum, Governor, args[1]);

while(shipno = do_shiplist(&s, &nextshipno))
    if(in_list(Playernum, args[1], s, &nextshipno) &&
       authorized(Governor, s)) {
	if (!speed_rating(s) && landed(s)) {
	    sprintf(buf,"That ship is not designed to be launched.\n");
	    notify(Playernum, Governor, buf);
	    free(s);
	    continue;
	}

	if (!(s->docked || s->whatorbits==LEVEL_SHIP)) {
	    sprintf(buf,"%s is not landed or docked.\n", Ship(s));
	    notify(Playernum, Governor, buf);
	    free(s);
	    continue;
	}
	if(!landed(s)) APcount = 0;
	if(landed(s) && s->resource > Max_resource(s)) {
	    sprintf(buf,"%s is too overloaded to launch.\n", Ship(s));
	    notify(Playernum, Governor, buf);
	    free(s);
	    continue;
	}
	if (s->whatorbits==LEVEL_SHIP) {
	    /* Factories cannot be launched once turned on. Maarten */
	    if (s->type==OTYPE_FACTORY && s->on) {
		notify(Playernum, Governor, "Factories cannot be launched once turned on.\n");
		notify(Playernum, Governor, "Consider using 'scrap'.\n");
		free(s);
		continue;
	    }
	    sh2 = s->destshipno; 
	    (void)getship(&s2, sh2);
	    if(landed(s2)) {
		remove_sh_ship(s, s2);
		getplanet(&p, (int)s2->storbits,(int)s2->pnumorbits);
		insert_sh_plan(p, s);
		putplanet(p, (int)s2->storbits,(int)s2->pnumorbits);
		s->storbits = s2->storbits;
		s->pnumorbits = s2->pnumorbits;
		s->destpnum = s2->pnumorbits;
		s->deststar = s2->deststar;
		s->xpos = s2->xpos;
		s->ypos = s2->ypos;
		s->land_x = s2->land_x;
		s->land_y = s2->land_y;
		s->docked = 1;
		s->whatdest = LEVEL_PLAN;
		s2->mass -= s->mass;
		s2->hanger -= Size(s);
		sprintf(buf, "Landed on %s/%s.\n", Stars[s->storbits]->name, 
			Stars[s->storbits]->pnames[s->pnumorbits]);
		notify(Playernum, Governor, buf);
		free(p);
		putship(s);
		putship(s2);
	    } else if(s2->whatorbits==LEVEL_PLAN) {
		remove_sh_ship(s, s2);
		sprintf(buf, "%s launched from %s.\n", Ship(s), Ship(s2));
		notify(Playernum, Governor, buf);
		s->xpos = s2->xpos;
		s->ypos = s2->ypos;
		s->docked = 0;
		s->whatdest = LEVEL_UNIV;
		s2->mass -= s->mass;
		s2->hanger -= Size(s);
		getplanet(&p, (int)s2->storbits,(int)s2->pnumorbits);
		insert_sh_plan(p, s);
		s->storbits = s2->storbits;
		s->pnumorbits = s2->pnumorbits;
		putplanet(p, (int)s2->storbits,(int)s2->pnumorbits);
		sprintf(buf, "Orbiting %s/%s.\n", Stars[s->storbits]->name, 
			Stars[s->storbits]->pnames[s->pnumorbits]);
		notify(Playernum, Governor, buf);
		free(p);
		putship(s);
		putship(s2);
	    } else if(s2->whatorbits==LEVEL_STAR) {
		remove_sh_ship(s, s2);
		sprintf(buf, "%s launched from %s.\n", Ship(s), Ship(s2));
		notify(Playernum, Governor, buf);
		s->xpos = s2->xpos;
		s->ypos = s2->ypos;
		s->docked = 0;
		s->whatdest = LEVEL_UNIV;
		s2->mass -= s->mass;
		s2->hanger -= Size(s);
		getstar(&(Stars[s2->storbits]), (int)s2->storbits);
		insert_sh_star(Stars[s2->storbits], s);
		s->storbits = s2->storbits;
		putstar(Stars[s2->storbits], (int)s2->storbits);
		sprintf(buf, "Orbiting %s.\n", Stars[s->storbits]->name);
		notify(Playernum, Governor, buf);
		putship(s);
		putship(s2);
	    } else if(s2->whatorbits==LEVEL_UNIV) {
		remove_sh_ship(s, s2);
		sprintf(buf, "%s launched from %s.\n", Ship(s), Ship(s2));
		notify(Playernum, Governor, buf);
		s->xpos = s2->xpos;
		s->ypos = s2->ypos;
		s->docked = 0;
		s->whatdest = LEVEL_UNIV;
		s2->mass -= s->mass;
		s2->hanger -= Size(s);
		getsdata(&Sdata);
		insert_sh_univ(&Sdata, s);
		notify(Playernum, Governor, "Universe level.\n");
		putsdata(&Sdata);
		putship(s);
		putship(s2);
	    } else {
		notify(Playernum, Governor, "You can't launch that ship.\n");
		free(s2);
		free(s);
		continue;
	    }
	    free(s2);
	    free(s);
	} else if (s->whatdest==LEVEL_SHIP) {
	    sh2 = s->destshipno; 
	    (void)getship(&s2, sh2);
	    if (s2->whatorbits==LEVEL_UNIV) {
		if (!enufAP(Playernum,Governor,Sdata.AP[Playernum-1], APcount)) { 
		    free(s);
		    free(s2);
		    continue;
		} else
		    deductAPs(Playernum, Governor, APcount, 0, 1);
	    } else {
		if (!enufAP(Playernum,Governor,Stars[s->storbits]->AP[Playernum-1], APcount)) {
		    free(s);
		    free(s2);
		    continue;
		} else
		    deductAPs(Playernum, Governor, APcount, (int)s->storbits, 0);
	    }
	    s->docked = 0;
	    s->whatdest = LEVEL_UNIV;
	    s->destshipno = 0;
	    s2->docked = 0;
	    s2->whatdest = LEVEL_UNIV;
	    s2->destshipno = 0;
	    sprintf(buf,"%s undocked from %s.\n",  Ship(s), Ship(s2));
	    notify(Playernum, Governor, buf);
	    putship(s);
	    putship(s2);
	    free(s);
	    free(s2);
	} else {
	    if (!enufAP(Playernum,Governor,Stars[s->storbits]->AP[Playernum-1], APcount)) { 
		free(s);
		return;
	    } else
		deductAPs(Playernum, Governor, APcount, (int)s->storbits, 0);

	/* adjust x,ypos to absolute coords */
	    getplanet(&p, (int)s->storbits,(int)s->pnumorbits);
	    sprintf(buf,"Planet /%s/%s has gravity field of %.2f\n", Stars[s->storbits]->name,
		    Stars[s->storbits]->pnames[s->pnumorbits], gravity(p));
	    notify(Playernum, Governor, buf);
	    s->xpos = Stars[s->storbits]->xpos + p->xpos + 
		(double)int_rand((int)(-DIST_TO_LAND/4),(int)(DIST_TO_LAND/4));
	    s->ypos = Stars[s->storbits]->ypos + p->ypos + 
		(double)int_rand((int)(-DIST_TO_LAND/4),(int)(DIST_TO_LAND/4));

	/* subtract fuel from ship */
	    fuel = gravity(p) * s->mass * LAUNCH_GRAV_MASS_FACTOR;
	    if (s->fuel < fuel) {
		sprintf(buf,"%s does not have enough fuel! (%.1f)\n",
			Ship(s), fuel);
		notify(Playernum, Governor, buf);
		free(p);
		free(s);
		return;
	    }
	    use_fuel(s, fuel);
	    s->docked = 0;
	    s->whatdest = LEVEL_UNIV;	/* no destination */
	    switch(s->type) {
	      case OTYPE_CANIST:
	      case OTYPE_GREEN:
		s->special.timer.count = 0; break;
	      default:
		break;
	    }
	    s->notified = 0;
	    putship(s);
	    if (!p->explored) {
	    /* not yet explored by owner; space exploration causes the
	       player to see a whole map */
		p->explored = 1;
		putplanet(p,(int)s->storbits,(int)s->pnumorbits);
	    }
	    sprintf(buf,"%s observed launching from planet /%s/%s.\n",
		    Ship(s), Stars[s->storbits]->name,
		    Stars[s->storbits]->pnames[s->pnumorbits]);
	    for (i=1; i<=Num_races; i++)
		if (p->info[i-1].numsectsowned && i!=Playernum)
		    notify(i, (int)Stars[s->storbits]->governor[i-1], buf);
	    free(p);

	    sprintf(buf,"%s launched from planet,", Ship(s));
	    notify(Playernum, Governor, buf);
	    sprintf(buf," using %.1f fuel.\n",fuel);
	    notify(Playernum, Governor, buf);

	    switch(s->type) {
	      case OTYPE_CANIST:
		notify(Playernum, Governor, "A cloud of dust envelopes your planet.\n");
		break;
	      case OTYPE_GREEN:
		notify(Playernum, Governor, "Green house gases surround the planet.\n");
		break;
	      default: break;
	    }
	    free(s);
	}
    } else
	free(s);
}
