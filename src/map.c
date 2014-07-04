/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  map.c -- display sector map of current planet
 */

#define DISP_DATA 1

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
#include <curses.h>
#include <string.h>

racetype *Race;
extern char *Planet_types[];

void map(int, int, int);
void show_map(int, int, int, int, planettype *, int, int);
char desshow(int, int, planettype *, int, int, racetype *);
#include "getplace.p"
#include "GB_server.p"
#include "files_shl.p"
#include "fire.p"
#include "orbit.p"
#include "shlmisc.p"
#include "max.p"
#include "rand.p"

void map(int Playernum, int Governor, int APcount)
{
   planettype *p;
   placetype where;

   where = Getplace(Playernum, Governor, args[1],0);

   if (where.err) return;
   else if (where.level==LEVEL_SHIP) {
       notify(Playernum, Governor, "Bad scope.\n");
       return;
   } else if (where.level==LEVEL_PLAN) {
       getplanet(&p,(int)where.snum,(int)where.pnum);
       show_map(Playernum, Governor, (int)where.snum, (int)where.pnum,
		p, DISP_DATA, 0);	
       free(p);
       if (Stars[where.snum]->stability > 50)
	   notify(Playernum, Governor, "WARNING! This planet's primary is unstable.\n");
   } else
       orbit(Playernum, Governor, APcount);	/* make orbit map instead */
}

void show_map(int Playernum, int Governor, int snum, int pnum, planettype *p,
	      int show, int iq)
{
   reg int x,y,i,f=0,owner, owned1;
   int sh;
   shiptype *s;
   char shiplocs[MAX_X][MAX_Y];
   hugestr output;

   bzero((char *)shiplocs, sizeof(shiplocs));

   Race = races[Playernum-1];
   getsmap(Smap, p);
   if(!Race->governor[Governor].toggle.geography) {
       /* traverse ship list on planet; find out if we can look at 
	  ships here. */
       iq = !!p->info[Playernum-1].numsectsowned;
       sh = p->ships;
	
       while (sh) {
	   if(!getship(&s, sh)) {
	       sh = 0; continue;
	   }
	   if(s->owner == Playernum && authorized(Governor, s) &&
	      (s->popn || (s->type==OTYPE_PROBE))) iq = 1;
	   if (s->alive && landed(s))
	       shiplocs[s->land_x][s->land_y] = Shipltrs[s->type];
	   sh = s->nextship;
	   free(s);
       }
   }
/* report that this is a planet map */
   sprintf(output, "$");

   sprintf(buf, "%s;", Stars[snum]->pnames[pnum]);	
   strcat(output, buf);

   sprintf(buf, "%d;%d;%d;", p->Maxx, p->Maxy,show);
   strcat(output, buf);

/* send map data */
   for(y=0; y<p->Maxy; y++)
       for (x=0; x<p->Maxx; x++) {
	   owner = Sector(*p, x, y).owner;
	   owned1 = (owner == Race->governor[Governor].toggle.highlight);
	   if (shiplocs[x][y] && iq) {
	       if(Race->governor[Governor].toggle.color)
		   sprintf(buf,"%c%c", (char)(owner+'?'), shiplocs[x][y]);
	       else {
		   if(owned1 && Race->governor[Governor].toggle.inverse)
		       sprintf(buf,"1%c", shiplocs[x][y]);
		   else
		       sprintf(buf,"0%c", shiplocs[x][y]);
	       }
	   } else {
	       if(Race->governor[Governor].toggle.color)
		   sprintf(buf,"%c%c", (char)(owner+'?'),
			   desshow(Playernum,Governor, p, x, y, Race));
	       else {
		   if(owned1 && Race->governor[Governor].toggle.inverse)
		       sprintf(buf,"1%c", desshow(Playernum,Governor, p,x,y, Race));
		   else
		       sprintf(buf,"0%c", desshow(Playernum,Governor, p,x,y, Race));
	       }
	   }
	   strcat(output, buf);
       }
   strcat(output, "\n");
   notify(Playernum, Governor, output);

   if (show){
       sprintf(temp, "Type: %8s   Sects %7s: %3u   Aliens:",
	       Planet_types[p->type], Race->Metamorph ? "covered" : "owned",
	       p->info[Playernum-1].numsectsowned);
       if (p->explored || Race->tech >= TECH_EXPLORE) {
	   f=0;
	   for (i=1; i<MAXPLAYERS; i++)
	       if (p->info[i-1].numsectsowned && i!=Playernum) {
		   f=1;
		   sprintf(buf, "%c%d", isset(Race->atwar,i) ? '*' : ' ', i);
		   strcat(temp, buf);
	       }
	   if (!f) strcat(temp, "(none)");
       } else
	   strcat(temp, "\?\?\?");
       strcat(temp, "\n");
       notify(Playernum, Governor, temp);
       sprintf(temp, "              Guns : %3d             Mob Points : %ld\n",
	       p->info[Playernum-1].guns,
	       p->info[Playernum-1].mob_points);
       notify(Playernum, Governor, temp);
       sprintf(temp, "      Mobilization : %3d (%3d)     Compatibility: %.2f%%",
		p->info[Playernum-1].comread,
		p->info[Playernum-1].mob_set,
		compatibility(p,Race));
       if(p->conditions[TOXIC]>50) {
	   sprintf(buf,"    (%d%% TOXIC)",p->conditions[TOXIC]);
	   strcat(temp, buf);
       }
       strcat(temp, "\n");
       notify(Playernum, Governor, temp);
       sprintf(temp, "Resource stockpile : %-9u    Fuel stockpile: %u\n",
	       p->info[Playernum-1].resource,
	       p->info[Playernum-1].fuel);
       notify(Playernum, Governor, temp);
       sprintf(temp, "      Destruct cap : %-9u%18s: %-5lu (%lu/%u)\n",
	       p->info[Playernum-1].destruct,
	       Race->Metamorph ? "Tons of biomass" : "Total Population", 
	       p->info[Playernum-1].popn, p->popn, 
	       round_rand(.01*(100.-p->conditions[TOXIC])*p->maxpopn) );
       notify(Playernum, Governor, temp);
       sprintf(temp, "          Crystals : %-9u%18s: %-5lu (%lu)\n",
	       p->info[Playernum-1].crystals,
	       "Ground forces", p->info[Playernum-1].troops, p->troops);
       notify(Playernum, Governor, temp);
       sprintf(temp, "%ld Total Resource Deposits     Tax rate %u%%  New %u%%\n",
	       p->total_resources, p->info[Playernum-1].tax,
	       p->info[Playernum-1].newtax);
       notify(Playernum, Governor, temp);
       sprintf(temp, "Estimated Production Next Update : %.2f\n",
	       p->info[Playernum-1].est_production);
       notify(Playernum, Governor, temp);
       if(p->slaved_to) {
	   sprintf(temp, "      ENSLAVED to player %d\n", p->slaved_to);
	   notify(Playernum, Governor, temp);
       }
   }
}

char desshow(int Playernum, int Governor, planettype *p,
	     int x, int y, racetype *r)
{
reg sectortype *s;

s = &Sector(*p,x,y);

if(s->troops && !r->governor[Governor].toggle.geography) {
    if(s->owner==Playernum) return CHAR_MY_TROOPS;
    else if(isset(r->allied, s->owner)) return CHAR_ALLIED_TROOPS;
    else if(isset(r->atwar, s->owner)) return CHAR_ATWAR_TROOPS;
    else return CHAR_NEUTRAL_TROOPS;
}

if(s->owner && !r->governor[Governor].toggle.geography &&
   !r->governor[Governor].toggle.color) {
    if(!r->governor[Governor].toggle.inverse ||
       s->owner != r->governor[Governor].toggle.highlight) {
	if(!r->governor[Governor].toggle.double_digits)
	    return s->owner %10 + '0'; 
	else {
	    if (s->owner < 10 || x % 2)
		return s->owner % 10 + '0';
	    else
		return s->owner / 10 + '0';
	}
    }
}


if (s->crystals && (r->discoveries[D_CRYSTAL] || r->God))
    return CHAR_CRYSTAL;

switch (s->condition) {
  case WASTED: return CHAR_WASTED;
  case SEA: return CHAR_SEA;
  case LAND: return CHAR_LAND;
  case MOUNT: return CHAR_MOUNT;
  case GAS: return CHAR_GAS;
  case PLATED: return CHAR_PLATED;
  case ICE: return CHAR_ICE;
  case DESERT: return CHAR_DESERT;
  case FOREST: return CHAR_FOREST;
  default: return('?');
}
}

