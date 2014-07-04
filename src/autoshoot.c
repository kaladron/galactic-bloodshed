/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB.c, enroll.dat.
 * Restrictions in GB.c.
 * autoshoot() -- shoot <-> retaliate routine
 * Bombard() -- ship bombards planet
 */

#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "doturn.h"
#include "buffers.h"

extern long Shipdata[NUMSTYPES][NUMABILS];

int Bombard(shiptype *, planettype *, racetype *);
#include "files_shl.p"
#include "GB_server.p"
#include "max.p"
#include "perm.p"
#include "shootblast.p"
#include "teleg_send.p"

/* ship #shipno bombards planet, then alert whom it may concern.
 */
int Bombard(shiptype *ship, planettype *planet, racetype *r)
{
shiptype *s;
int x,y,x2= -1,y2,oldown,numdest=0,found=0;
int i, sh;
int ok;

/* for telegramming */
bzero((char *)Nuked, sizeof(Nuked));

/* check to see if PDNs are present */
ok = 1;
sh = planet->ships;
while(sh && ok) {
    (void)getship(&s, sh);
    ok = !(s->alive && s->type==OTYPE_PLANDEF && s->owner != ship->owner);
    sh = s->nextship;
    free(s);
}
if(!ok) {
    sprintf(buf, "Bombardment of %s cancelled, PDNs are present.\n",
	    prin_ship_orbits(ship));
    warn((int)ship->owner, (int)ship->governor, buf);
    return 0;
}

  getsmap(Smap, planet);

 /* look for someone to bombard-check for war */
  (void)Getxysect(planet,0,0,1);	/* reset */
  while (!found && Getxysect(planet,&x,&y,0)) {
	if (Sector(*planet,x,y).owner && Sector(*planet,x,y).owner!=ship->owner
	    && (Sector(*planet,x,y).condition!=WASTED)) {
	  if (isset(r->atwar, Sector(*planet,x,y).owner) ||
	      (ship->type==OTYPE_BERS && 
	       Sector(*planet,x,y).owner==ship->special.mind.target))
	      found=1;
	  else
	      x2=x,y2=y;
      }
    }
  if (x2 != -1) {
	x = x2;		/* no one we're at war with; bomb someone else. */
	y = y2;
	found = 1;
  }

  if (found) {
	int str;
    str = MIN(Shipdata[ship->type][ABIL_GUNS]*(100-ship->damage)/100., ship->destruct);
	/* save owner of destroyed sector */
    if (str) {
	bzero(Nuked, sizeof(Nuked));
    	oldown = Sector(*planet,x,y).owner;
    	ship->destruct -= str;
    	ship->mass -= str * MASS_DESTRUCT;

	numdest = shoot_ship_to_planet(ship, planet, str,
				       x, y, 0, 0, 0, long_buf, short_buf);
	/* (0=dont get smap) */
	if(numdest < 0) numdest = 0;

      	/* tell the bombarding player about it.. */
    	sprintf(telegram_buf,"REPORT from ship #%d\n\n",ship->number);
	strcat(telegram_buf, short_buf);
    	sprintf(buf,"sector %d,%d (owner %d).  %d sectors destroyed.\n",
			x, y, oldown, numdest);
	strcat(telegram_buf, buf);
	notify((int)ship->owner, (int)ship->governor, telegram_buf); 

     /* notify other player. */
    	sprintf(telegram_buf,"ALERT from planet /%s/%s\n",
			Stars[ship->storbits]->name,
			Stars[ship->storbits]->pnames[ship->pnumorbits]);
     	sprintf(buf,"%c%d %s bombarded sector %d,%d; %d sectors destroyed.\n",
 		Shipltrs[ship->type], ship->number, ship->name, x, y, numdest);
	strcat(telegram_buf, buf);
	sprintf(buf, "%c%d %s [%d] bombards %s/%s\n",
		Shipltrs[ship->type], ship->number, ship->name, ship->owner,
		Stars[ship->storbits]->name,
		Stars[ship->storbits]->pnames[ship->pnumorbits]);
	for (i=1; i<=Num_races; i++)
	   if (Nuked[i-1] && i != ship->owner)
		warn(i, (int)Stars[ship->storbits]->governor[i-1], telegram_buf);
	post(buf, COMBAT);

/* enemy planet retaliates along with defending forces */
    } else {
		/* no weapons! */
	if (!ship->notified) {
		ship->notified = 1;
		sprintf(telegram_buf, 
 		  "Bulletin\n\n %c%d %s has no weapons to bombard with.\n",
 		   Shipltrs[ship->type],ship->number,ship->name);
		warn((int)ship->owner, (int)ship->governor, telegram_buf);
	    }
    }

    putsmap(Smap, planet);

  } else {
	 /* there were no sectors worth bombing. */
    if (!ship->notified) {
	ship->notified = 1;
     	sprintf(telegram_buf,"Report from %c%d %s\n\n",
		Shipltrs[ship->type], ship->number,ship->name);
    	sprintf(buf,"Planet /%s/%s has been saturation bombed.\n",
		Stars[ship->storbits]->name,
		Stars[ship->storbits]->pnames[ship->pnumorbits]);
	strcat(telegram_buf, buf);   
	notify((int)ship->owner, (int)ship->governor, telegram_buf); 
    }
}
  return numdest;

}
