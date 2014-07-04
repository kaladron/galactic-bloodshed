int Sectormappos;
/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *  doplanet.c -- do one turn on a planet.
 */

#include <math.h>
/*#include <malloc.h>*/

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "doturn.h"
#include "power.h"
#include "buffers.h"

extern long Shipdata[NUMSTYPES][NUMABILS];

/* types of accidents that can occur on a toxic planet. */
char *Accidents_uninhab[] = {
  /* on water sectors */
  "Widespread waste spill",
  "Ecological instability",
  /* on land sectors */
  "Massive volcanic eruptions",
  "Ecological instability",
  /* on mountain sectors */
  "Massive volcanic eruptions",
  "Ecological instability",
  /* gas sectors */
  "Lethal toxin concentration",
  "Ecological instability",
  /* ice */
  "Leak in isolated chemical plant",
  "Continental warming cause glacial melting",
  /* plate */
  "Nuclear accident",
  "Untended nuclear plant explodes"
};

char *Accidents_inhab[] = {
  "Nuclear accident",
  "Terrorists trigger nuclear weapon",
  "Release of toxic waste",
  "Weapons dump explosion",
  "Massive starvation",
  "Virus epidemic",
  "famine",
  "starvation",
  "Widespread cultist suicide",
  "Atomic experiment gone wrong",
  "Great Hrung collapse"
};

int doplanet(int, planettype *, int);
int moveship_onplanet(shiptype *, planettype *);
void terraform(shiptype *, planettype *);
void plow(shiptype *, planettype *);
void do_dome(shiptype *, planettype *);
void do_quarry(shiptype *, planettype *);
void do_berserker(shiptype *, planettype *);
void do_recover(planettype *, int, int);
double est_production(sectortype *);
#include "files_shl.p"
#include "perm.p"
#include "max.p"
#include "VN.p"
#include "fire.p"
#include "moveship.p"
#include "doship.p"
#include "load.p"
#include "dosector.p"
#include "rand.p"
#include "teleg_send.p"
#include "build.p"
#include "GB_server.p"
#include "shootblast.p"
#include "autoshoot.p"
#include "tech.p"
#include "move.p"
#include "shlmisc.p"

int doplanet(int starnum, planettype *planet, int planetnum)
{
int shipno,x,y,nukex,nukey;
int o=0;
char *nukem;
reg int i;
sectortype *p;
shiptype *ship;
double fadd;
int timer=20;
int oldplanetpopn, oldplanetmaxpopn;
unsigned char allmod=0,allexp=0;

Sectormappos = planet->sectormappos;
/*if (!(Stars[starnum]->inhabited[0]+Stars[starnum]->inhabited[1]))
    return 0;  /* no one's here now */

getsmap(Smap,planet);
PermuteSects(planet);
bzero((char *)Sectinfo, sizeof(Sectinfo) );

bzero((char *)avg_mob, sizeof(avg_mob) );
bzero((char *)sects_gained, sizeof(sects_gained) );
bzero((char *)sects_lost, sizeof(sects_lost) );
bzero((char *)prod_res, sizeof(prod_res) );
bzero((char *)prod_fuel, sizeof(prod_fuel) );
bzero((char *)prod_destruct, sizeof(prod_destruct) );
bzero((char *)prod_crystals, sizeof(prod_crystals) );

tot_resdep = prod_eff= prod_mob = tot_captured = 0;
Claims = 0;

oldplanetmaxpopn = planet->maxpopn;
planet->maxpopn = 0;

oldplanetpopn = planet->popn;
planet->popn = 0;  /* initialize population for recount */
planet->troops = 0;
planet->total_resources = 0;

/* reset global variables */
for (i=1; i<=Num_races; i++) {
    Compat[i-1] = compatibility(planet, races[i-1]);
    planet->info[i-1].numsectsowned = 0;
    planet->info[i-1].troops = 0;
    planet->info[i-1].popn = 0;
    planet->info[i-1].est_production = 0.0;
    prod_crystals[i-1] = 0;
    prod_fuel[i-1] = 0;
    prod_destruct[i-1] = 0;
    prod_res[i-1] = 0;
    avg_mob[i-1] = 0;
}

shipno = planet->ships;
while (shipno) {
    ship = ships[shipno];
    if(ship->alive && !ship->rad) {
/* planet level functions - do these here because they use the sector map
	or affect planet production */
	switch(ship->type) {
	  case OTYPE_VN:
	    planet_doVN(ship, planet); break;
	  case OTYPE_BERS:
	    if(!ship->destruct || !ship->bombard)
		planet_doVN(ship, planet);
	    else
		do_berserker(ship, planet);
	    break;
	  case OTYPE_TERRA:
	    if (ship->on && landed(ship) && ship->popn)
		if(ship->fuel >= (double)FUEL_COST_TERRA)
		    terraform(ship, planet);
		else if (!ship->notified) {
		    ship->notified = 1;
		    msg_OOF(ship);
		}
	    break;
	  case OTYPE_PLOW:
	    if (ship->on && landed(ship)) {
		if(ship->fuel >= (double)FUEL_COST_PLOW)
		    plow(ship, planet);
		else if (!ship->notified) {
		    ship->notified = 1;
		    msg_OOF(ship);
		}
	    } else if (ship->on) {
		sprintf(buf, "K%d is not landed.", ship->number);
		push_telegram(ship->owner, ship->governor, buf);
	    } else {
		sprintf(buf, "K%d is not switched on.", ship->number);
		push_telegram(ship->owner, ship->governor, buf);
	    }
	    break;
	  case OTYPE_DOME:
	    if (ship->on && landed(ship)) {
		if(ship->resource >= RES_COST_DOME)
		    do_dome(ship, planet);
		else {
		    sprintf(buf, "Y%d does not have enough resources.", ship->number);
		    push_telegram(ship->owner, ship->governor, buf);
		}
	    } else if (ship->on) {
		sprintf(buf, "Y%d is not landed.", ship->number);
		push_telegram(ship->owner, ship->governor, buf);
	    } else {
		sprintf(buf, "Y%d is not switched on.", ship->number);
		push_telegram(ship->owner, ship->governor, buf);
	    }
	    break;
	  case OTYPE_WPLANT:
	    if(landed(ship))
		if(ship->resource >= RES_COST_WPLANT
		   && ship->fuel >= FUEL_COST_WPLANT)
		    prod_destruct[ship->owner-1] += do_weapon_plant(ship);
		else {
		    if (ship->resource < RES_COST_WPLANT) {
		        sprintf(buf, "W%d does not have enough resources.", ship->number);
		        push_telegram(ship->owner, ship->governor, buf);
		    } else {
		        sprintf(buf, "W%d does not have enough fuel.", ship->number);
		        push_telegram(ship->owner, ship->governor, buf);
		    }
		}
	    else {
		sprintf(buf, "W%d is not landed.", ship->number);
		push_telegram(ship->owner, ship->governor, buf);
	    }
	    break;
	  case OTYPE_QUARRY:
	    if(ship->on && landed(ship) && ship->popn)
		if(ship->fuel >= FUEL_COST_QUARRY)
		    do_quarry(ship, planet);
		else if (!ship->notified) {
		    ship->on = 0;
		    msg_OOF(ship);
		}
	    else {
		if (!ship->on) {
			sprintf(buf, "q%d is not switched on.", ship->number);
		}
		if (!landed(ship)) {
			sprintf(buf, "q%d is not landed.", ship->number);
		}
		if (!ship->popn) {
			sprintf(buf, "q%d does not have workers aboard.", ship->number);
		}
		push_telegram(ship->owner, ship->governor, buf);
	    }
	    break;
	}
/* add fuel for ships orbiting a gas giant */
	if(!landed(ship) && planet->type==TYPE_GASGIANT) {
	    switch(ship->type) {
	      case STYPE_TANKER:
		fadd = FUEL_GAS_ADD_TANKER; break;
	      case STYPE_HABITAT:
		fadd = FUEL_GAS_ADD_HABITAT; break;
	      default:
		fadd = FUEL_GAS_ADD; break;
	    }
	    fadd = MIN((double)Max_fuel(ship)-ship->fuel, fadd);
	    rcv_fuel(ship, fadd);
	}
    }
    shipno = ship->nextship;
}

/*if (!Stinfo[starnum][planetnum].inhab)
    return 0;  /* (no one's explored the planet) */

  /* check for space mirrors (among other things) warming the planet */
  /* if a change in any artificial warming/cooling trends */
planet->conditions[TEMP] = planet->conditions[RTEMP] +
    Stinfo[starnum][planetnum].temp_add + int_rand(-5,5);


(void)Getxysect(planet, &x, &y, 1);

while (Getxysect(planet, &x, &y, 0)) {
    p = &Sector(*planet,x,y);

    if (p->owner && (p->popn || p->troops)) {
	allmod = 1;
	if (!Stars[starnum]->nova_stage) {
	    produce(Stars[starnum],planet,p);
	    if(p->owner)
		planet->info[p->owner-1].est_production += est_production(p);
	    spread(planet,p,x,y);
	} else {
	    /* damage sector from supernova */
	    p->resource++;
	    p->fert *= 0.8;
	    if (Stars[starnum]->nova_stage == 14)
		p->popn = p->owner = p->troops = 0;
	    else
		p->popn = round_rand((double)p->popn * .50);
	}
	Sectinfo[x][y].done = 1;
    }

    if((!p->popn && !p->troops) || !p->owner) {
	p->owner = 0;
	p->popn = p->troops = 0;
    }

/*    
    if (p->wasted) {
	if (x>1 && x<planet->Maxx-2) {
	    if (p->des==DES_SEA || p->des==DES_GAS) {
		if ( y>1 && y<planet->Maxy-2 && 
		    (!(p-1)->wasted || !(p+1)->wasted) && !random()%5)
		    p->wasted = 0;
	    } else if (p->des==DES_LAND || p->des==DES_MOUNT
		       || p->des==DES_ICE) {
		if ( y>1 && y<planet->Maxy-2 && ((p-1)->popn || (p+1)->popn)
		    && !random()%10)
		    p->wasted = 0;
	    }
	}
    }
*/
/*
    if (Stars[starnum]->nova_stage) {
	if (p->des==DES_ICE)
	    if(random()&01)
		p->des = DES_LAND;
	    else if (p->des==DES_SEA)
		if(random()&01)
		    if ( (x>0 && (p-1)->des==DES_LAND) ||
			(x<planet->Maxx-1 && (p+1)->des==DES_LAND) || 
			(y>0 && (p-planet->Maxx)->des==DES_LAND) ||
			(y<planet->Maxy-1 && (p+planet->Maxx)->des==DES_LAND ) ) {
			p->des = DES_LAND;
			p->popn = p->owner = p->troops = 0;
			p->resource += int_rand(1,5);
			p->fert = int_rand(1,4);
		    }
		    }
		    */
}


(void)Getxysect(planet, &x, &y, 1);
while (Getxysect(planet, &x, &y, 0)) {
    p = &Sector(*planet,x,y);
    if (p->owner) planet->info[p->owner-1].numsectsowned++;
}

if (planet->expltimer >= 1)
    planet->expltimer--;
if (!Stars[starnum]->nova_stage && !planet->expltimer) {
    if (!planet->expltimer) planet->expltimer = 5;
    for (i=1; !Claims && !allexp && i<=Num_races; i++) {
	/* sectors have been modified for this player*/
	if (planet->info[i-1].numsectsowned)
	    while (!Claims && !allexp && timer>0) {
		timer -= 1;
		o = 1;
		(void)Getxysect(planet, &x, &y, 1);
		while (!Claims && Getxysect(planet, &x, &y, 0)) {
		    /* find out if all sectors have been explored */
		    o &= Sectinfo[x][y].explored;
		    p = &Sector(*planet,x,y);
		    if (( (Sectinfo[x][y].explored==i) && !(random()&02) )
			&& (!p->owner && p->condition!=WASTED
			    && p->condition==races[i-1]->likesbest) ) {
			/*  explorations have found an island */
			Claims = i;
			p->popn = races[i-1]->number_sexes;
			p->owner = i;
			tot_captured = 1;
		    } else
			explore(planet, p, x, y, i);
		}
		allexp |= o;  /* all sectors explored for this player */
	    }
    }
}

if (allexp)	planet->expltimer = 5;

  /* environment nukes a random sector */
if (planet->conditions[TOXIC] > ENVIR_DAMAGE_TOX) {
    nukex = int_rand(0,(int)planet->Maxx-1);
    nukey = int_rand(0,(int)planet->Maxy-1);
    p = &Sector(*planet,nukex,nukey);
    p->condition = WASTED;
    /* index into accident type array */
    nukem = p->popn + p->troops ? 
	Accidents_inhab[int_rand(0,sizeof(Accidents_inhab)/sizeof(char *))]
	    : Accidents_uninhab[p->type * 2 + (random()&01)];
    p->popn = p->owner = p->troops = 0;
}

for (i=1; i<=Num_races; i++)
    if (sects_gained[i-1] || sects_lost[i-1]) {
	sprintf(telegram_buf,"****** Report: Planet /%s/%s ******\n", 
		Stars[starnum]->name, Stars[starnum]->pnames[planetnum] );
	sprintf(buf," WAR STATUS: %d sectors gained, %d sectors lost.\n", 
		sects_gained[i-1],sects_lost[i-1]);
	strcat(telegram_buf, buf);
	push_telegram(i, (int)Stars[starnum]->governor[i-1], telegram_buf);
    }
for (i=1; i<=Num_races; i++) {
    planet->info[i-1].prod_crystals = prod_crystals[i-1];
    planet->info[i-1].prod_res = prod_res[i-1];
    planet->info[i-1].prod_fuel = prod_fuel[i-1];
    planet->info[i-1].prod_dest = prod_destruct[i-1];
    if (planet->info[i-1].autorep) {
	planet->info[i-1].autorep--;
	sprintf(telegram_buf,"\nFrom /%s/%s\n", 
		Stars[starnum]->name, Stars[starnum]->pnames[planetnum] );
	
	if (Stinfo[starnum][planetnum].temp_add) {
	    sprintf(buf,"Temp: %d to %d\n", 
		    planet->conditions[RTEMP], planet->conditions[TEMP]);
	    strcat(telegram_buf, buf);
	}
	sprintf(buf, "Total      Prod: %dr %df %dd\n", prod_res[i-1],
		prod_fuel[i-1], prod_destruct[i-1]);
	strcat(telegram_buf, buf);
	if(prod_crystals[i-1]) {
	    sprintf(buf, "    %d crystals found\n", prod_crystals[i-1]);
	    strcat(telegram_buf, buf);
	}
	if(tot_captured) {
	    sprintf(buf,"%d sectors captured\n", tot_captured);
	    strcat(telegram_buf, buf);
	}
	if (Stars[starnum]->nova_stage) {
	    sprintf(buf,"This planet's primary is in a Stage %d nova.\n", 
		    Stars[starnum]->nova_stage);
	    strcat(telegram_buf, buf);
	}
	/* remind the player that he should clean up the environment. */
	if (planet->conditions[TOXIC] > ENVIR_DAMAGE_TOX) {
	    sprintf(buf,"Environmental damage on sector %d,%d\n",
		    nukex, nukey);
	    strcat(telegram_buf, buf);
	}
	if (planet->slaved_to) {
	    sprintf(buf, "ENSLAVED to player %d\n", planet->slaved_to);
	    strcat(telegram_buf, buf);
	}
	push_telegram(i, Stars[starnum]->governor[i-1], telegram_buf);
    }
}

/* find out who is on this planet, for nova notification */
if (Stars[starnum]->nova_stage == 1) {
    sprintf(telegram_buf,"BULLETIN from /%s/%s\n",
	    Stars[starnum]->name, Stars[starnum]->pnames[planetnum]);
    sprintf(buf,"\nStar %s is undergoing nova.\n", Stars[starnum]->name);
    strcat(telegram_buf, buf);
    if (planet->type==TYPE_EARTH || planet->type==TYPE_WATER ||
	planet->type==TYPE_FOREST) {
	sprintf(buf,"Seas and rivers are boiling!\n");
	strcat(telegram_buf, buf);
    }
    sprintf(buf, "This planet must be evacuated immediately!\n%c",
	    TELEG_DELIM);
    strcat(telegram_buf, buf);
    for (i=1; i<=Num_races; i++)
	if (planet->info[i-1].numsectsowned)
	    push_telegram(i, Stars[starnum]->governor[i-1], telegram_buf);
}

do_recover(planet, starnum, planetnum);

planet->popn = 0;
planet->troops = 0;
planet->maxpopn = 0;
planet->total_resources = 0;

for (i=1; i<=Num_races; i++) {
    planet->info[i-1].numsectsowned = 0;
    planet->info[i-1].popn = 0;
    planet->info[i-1].troops = 0;
}

(void)Getxysect(planet, &x, &y, 1);
while (Getxysect(planet, &x, &y, 0)) {
    p = &Sector(*planet,x,y);
    if (p->owner) {
	planet->info[p->owner-1].numsectsowned++;
	planet->info[p->owner-1].troops += p->troops;
	planet->info[p->owner-1].popn += p->popn;
	planet->popn += p->popn;
	planet->troops += p->troops;
	planet->maxpopn += maxsupport(races[p->owner-1],p,
				      Compat[p->owner-1],planet->conditions[TOXIC]);
	Power[p->owner-1].troops += p->troops;
	Power[p->owner-1].popn += p->popn;
	Power[p->owner-1].sum_eff += p->eff;
	Power[p->owner-1].sum_mob += p->mobilization;
	starpopns[starnum][p->owner-1] += p->popn;
    } else {
	p->popn = 0;
	p->troops = 0;
    }
    planet->total_resources += p->resource;
}

/* deal with enslaved planets */
if (planet->slaved_to) {
    if (planet->info[planet->slaved_to-1].popn > planet->popn/1000) {
	for (i=1; i<=Num_races; i++)
	    /* add production to slave holder of planet */
	    if (planet->info[i-1].numsectsowned) {
		planet->info[planet->slaved_to-1].resource += prod_res[i-1];
		prod_res[i-1] = 0;
		planet->info[planet->slaved_to-1].fuel += prod_fuel[i-1];
		prod_fuel[i-1] = 0;
		planet->info[planet->slaved_to-1].destruct += prod_destruct[i-1];
		prod_destruct[i-1] = 0;
	    }
    } else {
	/* slave revolt! */
	/* first nuke some random sectors from the revolt */
	i = planet->popn / 1000 + 1;
	while (--i) {
	    p = &Sector(*planet,int_rand(0,(int)planet->Maxx-1),
			int_rand(0,(int)planet->Maxy-1));
	    if (p->popn + p->troops) {
		p->owner = p->popn = p->troops = 0;
		p->condition = WASTED;
	    }
	}
	/* now nuke all sectors belonging to former master */
	(void)Getxysect(planet, &x, &y, 1);
	while (Getxysect(planet, &x, &y, 0)) {
	    if (Stinfo[starnum][planetnum].intimidated && random()&01) {
		p =  &Sector(*planet,x,y);
		if (p->owner==planet->slaved_to) {
		    p->owner = 0;
		    p->popn = 0;
		    p->troops = 0;
		    p->condition = WASTED;
		}
	    }
	    /* also add up the populations while here */
	}
	sprintf(telegram_buf,"\nThere has been a SLAVE REVOLT on /%s/%s!\n",
		Stars[starnum]->name, Stars[starnum]->pnames[planetnum]);
	strcat(telegram_buf, buf);
	sprintf(buf,"All population belonging to player #%d on the planet have been killed!\n",planet->slaved_to);
	strcat(telegram_buf, buf);
	sprintf(buf,"Productions now go to their rightful owners.\n");
	strcat(telegram_buf, buf);
	for (i=1; i<=Num_races; i++)
	    if (planet->info[i-1].numsectsowned)
		push_telegram(i,
			      (int)Stars[starnum]->governor[i-1], telegram_buf);
	planet->slaved_to = 0;
    }
}
 
/* add production to all people here */
for (i=1; i<=Num_races; i++)
    if (planet->info[i-1].numsectsowned) {
   	planet->info[i-1].fuel += prod_fuel[i-1];
     	planet->info[i-1].resource += prod_res[i-1];
     	planet->info[i-1].destruct += prod_destruct[i-1];
     	planet->info[i-1].crystals += prod_crystals[i-1];

      /* tax the population - set new tax rate when done */
     	if(races[i-1]->Gov_ship) {
	    planet->info[i-1].prod_money = round_rand(INCOME_FACTOR*
						      (double)planet->info[i-1].tax * (double)planet->info[i-1].popn);
	    races[i-1]->governor[Stars[starnum]->governor[i-1]].money +=
		planet->info[i-1].prod_money;
	    planet->info[i-1].tax += MIN((int)planet->info[i-1].newtax
					 - (int)planet->info[i-1].tax, 5);
     	} else
	    planet->info[i-1].prod_money = 0;
     	races[i-1]->governor[Stars[starnum]->governor[i-1]].income +=
	    planet->info[i-1].prod_money;
     
	/* do tech investments */
     	if(races[i-1]->Gov_ship) {
	    if (races[i-1]->governor[Stars[starnum]->governor[i-1]].money >=
		planet->info[i-1].tech_invest) {
		planet->info[i-1].prod_tech =
		    tech_prod((int)(planet->info[i-1].tech_invest),
			      (int)(planet->info[i-1].popn));
		races[i-1]->governor[Stars[starnum]->governor[i-1]].money -=
		    planet->info[i-1].tech_invest;
		races[i-1]->tech += planet->info[i-1].prod_tech;
		races[i-1]->governor[Stars[starnum]->governor[i-1]].cost_tech +=
		    planet->info[i-1].tech_invest;
	    } else
		planet->info[i-1].prod_tech = 0;
     	} else
	    planet->info[i-1].prod_tech = 0;
      
	/* build wc's if it's been ordered */
	if (planet->info[i-1].tox_thresh>0 &&
	    planet->conditions[TOXIC] >= planet->info[i-1].tox_thresh &&
	    planet->info[i-1].resource>=Shipcost(OTYPE_TOXWC,races[i-1])) {
	    shiptype *s2; 
	    reg int t;
	    ++Num_ships;
	    ships = (shiptype **)realloc(ships,
					 (unsigned)((Num_ships+1)*sizeof(shiptype*)));
	    s2 = ships[Num_ships] = Malloc(shiptype);
	    bzero((char *)s2,sizeof(shiptype));
	    s2->number = Num_ships;
	    s2->type = OTYPE_TOXWC;

	    s2->armor = Shipdata[OTYPE_TOXWC][ABIL_ARMOR];
	    s2->guns = NONE;
	    s2->primary = Shipdata[OTYPE_TOXWC][ABIL_GUNS];
	    s2->primtype = Shipdata[OTYPE_TOXWC][ABIL_PRIMARY];
	    s2->secondary = Shipdata[OTYPE_TOXWC][ABIL_GUNS];
	    s2->sectype = Shipdata[OTYPE_TOXWC][ABIL_SECONDARY];
	    s2->max_crew = Shipdata[OTYPE_TOXWC][ABIL_MAXCREW];
	    s2->max_resource = Shipdata[OTYPE_TOXWC][ABIL_CARGO];
	    s2->max_fuel = Shipdata[OTYPE_TOXWC][ABIL_FUELCAP];
	    s2->max_destruct = Shipdata[OTYPE_TOXWC][ABIL_DESTCAP];
	    s2->max_speed = Shipdata[OTYPE_TOXWC][ABIL_SPEED];
	    s2->build_cost = Shipcost(OTYPE_TOXWC,races[i-1]);
	    s2->size = ship_size(s2);
	    s2->base_mass = 1.0; /* a hack */
	    s2->mass = s2->base_mass;
	    s2->alive = 1;
	    s2->active = 1;
	    sprintf(s2->name,"Scum%04d",Num_ships);

	    insert_sh_plan(planet, s2);

	    s2->whatorbits = LEVEL_PLAN;
	    s2->storbits = starnum;
	    s2->pnumorbits = planetnum;
	    s2->docked = 1;
	    s2->xpos = Stars[starnum]->xpos + planet->xpos;
	    s2->ypos = Stars[starnum]->ypos + planet->ypos;
	    s2->land_x = int_rand(0,(int)planet->Maxx-1);
	    s2->land_y = int_rand(0,(int)planet->Maxy-1);
	    s2->whatdest = LEVEL_PLAN;
	    s2->deststar = starnum;
	    s2->destpnum = planetnum;
	    s2->owner = i;
	    s2->governor = Stars[starnum]->governor[i-1];
	    t = MIN(TOXMAX,planet->conditions[TOXIC]);  /* amt of tox */
	    planet->conditions[TOXIC] -= t;
	    s2->special.waste.toxic = t;
	}
    }   /* (if numsectsowned[i]) */

if (planet->maxpopn > 0 && planet->conditions[TOXIC]<100)
    planet->conditions[TOXIC] += planet->popn/planet->maxpopn;

if (planet->conditions[TOXIC] > 100)
    planet->conditions[TOXIC] = 100;
else if (planet->conditions[TOXIC] < 0)
    planet->conditions[TOXIC] = 0;

for (i=1; i<=Num_races; i++) {
    Power[i-1].resource += planet->info[i-1].resource;
    Power[i-1].destruct += planet->info[i-1].destruct;
    Power[i-1].fuel += planet->info[i-1].fuel;
    Power[i-1].sectors_owned += planet->info[i-1].numsectsowned;
    Power[i-1].planets_owned += !!planet->info[i-1].numsectsowned;
    if (planet->info[i-1].numsectsowned) {
	/* combat readiness naturally moves towards the avg mobilization */
	planet->info[i-1].mob_points = avg_mob[i-1];
	avg_mob[i-1] /= (int)planet->info[i-1].numsectsowned;
	planet->info[i-1].comread = avg_mob[i-1];
    } else
	planet->info[i-1].comread = 0;
    planet->info[i-1].guns = planet_guns(planet->info[i-1].mob_points);
}
return allmod;
}

int moveship_onplanet(shiptype *ship, planettype *planet)
{
  int x, y, bounced = 0;

  if(ship->class[ship->special.terraform.index]=='s') {
      ship->on = 0;
      return 0;
  } else if(ship->class[ship->special.terraform.index]=='c')
      ship->special.terraform.index = 0;	/* reset the orders */
  
  (void)get_move(ship->class[ship->special.terraform.index],
		 ship->land_x, ship->land_y, &x, &y, planet) ;
  if (y >= planet->Maxy)
    bounced = 1, y -= 2 ; /* bounce off of south pole! */
  else if (y < 0)
    bounced = y = 1 ;     /* bounce off of north pole! */
  if (planet->Maxy == 1)
    y = 0 ;
  if (ship->class[ship->special.terraform.index+1] != '\0') {
      ++ship->special.terraform.index ;
      if ((ship->class[ship->special.terraform.index+1] == '\0') &&
	  (!ship->notified)) {
	  char teleg_buf[1000] ;
	  ship->notified = 1 ;
	  sprintf(teleg_buf, "%s is out of orders at %s.", Ship(ship),
		  prin_ship_orbits(ship));
	  push_telegram((int)(ship->owner),
			(int)ship->governor, teleg_buf);
      }
  } else if (bounced)
      ship->class[ship->special.terraform.index] +=
	  ((ship->class[ship->special.terraform.index] > '5') ? -6 : 6) ;
  ship->land_x = x ;
  ship->land_y = y ;
  return 1;
}

void terraform(shiptype *ship, planettype *planet)
{
  sectortype *s;
  
  /* move, and then terraform. */
  if(!moveship_onplanet(ship, planet)) return;
  s = &Sector(*planet, (int)ship->land_x, (int)ship->land_y) ;
  if ((s->condition != races[ship->owner-1]->likesbest) &&
      (s->condition != GAS) &&
      success((100-(int)ship->damage)*ship->popn/ship->max_crew)) {
      /* gas sectors can't be terraformed. */
      /* only condition can be terraformed, type doesn't change */
      s->condition = races[ship->owner-1]->likesbest;
      s->eff = 0 ;
      s->mobilization = 0 ;
      s->popn = s->troops = 0 ;
      s->owner = 0 ;
      use_fuel(ship, FUEL_COST_TERRA) ;
      if ((random()&01) && (planet->conditions[TOXIC] < 100))
	  planet->conditions[TOXIC] += 1 ;
      if ((ship->fuel < (double)FUEL_COST_TERRA) && (!ship->notified)) {
	  ship->notified = 1 ;
	  msg_OOF(ship) ;
      }
  } else if (s->condition == races[ship->owner-1]->likesbest) {
	sprintf(buf, " T%d is full of zealots!!!", ship->number);
	push_telegram(ship->owner, ship->governor, buf);
  }
  if (s->condition == GAS) {
	sprintf(buf, " T%d is trying to terraform gas.", ship->number);
	push_telegram(ship->owner, ship->governor, buf);
  }
}

void plow(shiptype *ship, planettype *planet)
{
  sectortype *s;
  
  if(!moveship_onplanet(ship, planet)) return;
  s = &Sector(*planet, (int)ship->land_x, (int)ship->land_y) ;
  if ((races[ship->owner-1]->likes[s->condition]) && (s->fert < 100)) {
      int adjust = round_rand(10 * (0.01*(100.0 - (double)ship->damage)
				    * (double)ship->popn)/ship->max_crew) ;
      if ((ship->fuel < (double)FUEL_COST_PLOW) && (!ship->notified)) {
	  ship->notified = 1 ;
	  msg_OOF(ship) ;
	  return;
      }
      s->fert = MIN(100, s->fert+adjust) ;
      if (s->fert >= 100) {
	  sprintf(buf, " K%d is full of zealots!!!", ship->number);
	  push_telegram(ship->owner, ship->governor, buf);
      }
      use_fuel(ship, FUEL_COST_PLOW) ;
      if ((random()&01) && (planet->conditions[TOXIC] < 100))
	  planet->conditions[TOXIC] += 1 ;
  }
}

void do_dome(shiptype *ship, planettype *planet)
{
  sectortype *s;
  int adjust;

  s = &Sector(*planet,(int)ship->land_x, (int)ship->land_y);
  if(s->eff >= 100) {
      sprintf(buf, " Y%d is full of zealots!!!", ship->number);
      push_telegram(ship->owner, ship->governor, buf);
      return;
  }
  adjust = round_rand(.05*(100.-(double)ship->damage)
		      * (double)ship->popn/ship->max_crew);
  s->eff += adjust;
  if(s->eff > 100) s->eff=100;
  use_resource(ship, RES_COST_DOME);
}

void do_quarry(shiptype *ship, planettype *planet)
{
    sectortype *s;
    int prod, tox;

    s = &Sector(*planet,(int)(ship->land_x),(int)(ship->land_y));

    if ((ship->fuel < (double)FUEL_COST_QUARRY)) {
      if (!ship->notified)
        msg_OOF(ship) ;
      ship->notified = 1 ;
      return;
    }
    /* nuke the sector */
    s->condition = WASTED;
    prod = round_rand(races[ship->owner-1]->metabolism
		      * (double)ship->popn/(double)ship->max_crew);
    ship->fuel -= FUEL_COST_QUARRY;
    prod_res[ship->owner-1] += prod;
    tox = int_rand(0,int_rand(0,prod));
    planet->conditions[TOXIC] = MIN(100, planet->conditions[TOXIC]+tox);
    if (s->fert>=prod)
      s->fert -= prod;
    else
      s->fert = 0;
}

void do_berserker(shiptype *ship, planettype *planet)
{
    if(ship->whatdest==LEVEL_PLAN && ship->whatorbits==LEVEL_PLAN &&
       !landed(ship) && ship->storbits==ship->deststar &&
       ship->pnumorbits==ship->destpnum) {
	if (!Bombard(ship, planet, races[ship->owner-1]))
	    ship->destpnum = int_rand(0, Stars[ship->storbits]->numplanets-1);
	else if (Sdata.VN_hitlist[ship->special.mind.who_killed-1]>0)
	    --Sdata.VN_hitlist[ship->special.mind.who_killed-1];
    }
}

void do_recover(planettype *planet, int starnum, int planetnum)
{
    int owners = 0, i, j;
    int ownerbits[2];
    int stolenres = 0, stolendes = 0, stolenfuel = 0, stolencrystals = 0;
    int all_buddies_here = 1;

    ownerbits[0] = ownerbits[1] = 0;
    
    for (i = 1; i <= Num_races && all_buddies_here; i++) {
	if (planet->info[i-1].numsectsowned > 0) {
	    owners++;
	    setbit(ownerbits, i);
	    for (j = 1; j < i && all_buddies_here; j++)
		if (isset(ownerbits,j) && (!isset(races[i-1]->allied, j) ||
					   !isset(races[j-1]->allied, i)))
		    all_buddies_here = 0;
	} else { /* Player i owns no sectors */
	    if (i != 1) { /* Can't steal from God */
		stolenres      += planet->info[i-1].resource;
		stolendes      += planet->info[i-1].destruct;
		stolenfuel     += planet->info[i-1].fuel;
		stolencrystals += planet->info[i-1].crystals;
	    }
	}
    }
    if (all_buddies_here && owners != 0 && (stolenres > 0 ||
					    stolendes > 0 || stolenfuel > 0 || stolencrystals > 0)) {
	/* Okay, we've got some loot to divvy up */
	int shares = owners;
	int res, des, fuel, crystals;
	int givenres = 0, givendes = 0, givenfuel = 0, givencrystals = 0;

	for (i = 1; i <= Num_races; i++)
	    if (isset(ownerbits,i)) {
		sprintf(telegram_buf,"Recovery Report: Planet /%s/%s\n", 
			Stars[starnum]->name, Stars[starnum]->pnames[planetnum] );
		push_telegram(i,
			      (int)Stars[starnum]->governor[i-1],
			      telegram_buf);
		sprintf(telegram_buf, "%-14.14s %5s %5s %5s %5s\n", 
			"", "res", "destr", "fuel", "xtal");
		push_telegram(i,
			      (int)Stars[starnum]->governor[i-1],
			      telegram_buf);
	    }
	/* First: give the loot the the conquerers */
	for (i = 1; i <= Num_races && owners > 1; i++)
	    if (isset(ownerbits,i)) { /* We have a winnah! */
		if ( (res = round_rand((double)stolenres / shares))
		    + givenres > stolenres)
		    res = stolenres - givenres;
		if ( (des = round_rand((double)stolendes / shares))
		    + givendes > stolendes)
		    des = stolendes - givendes;
		if ( (fuel = round_rand((double)stolenfuel / shares))
		    + givenfuel > stolenfuel)
		    fuel = stolenfuel - givenfuel;
		if ( (crystals = round_rand((double)stolencrystals / shares))
		    + givencrystals > stolencrystals)
		    crystals = stolencrystals - givencrystals;
		planet->info[i-1].resource += res; 
		givenres += res;
		planet->info[i-1].destruct += des; 
		givendes += des;
		planet->info[i-1].fuel += fuel;    
		givenfuel += fuel;
		planet->info[i-1].crystals += crystals; 
		givencrystals += crystals;

		owners--;
		sprintf(telegram_buf, "%-14.14s %5d %5d %5d %5d",
			races[i-1]->name, res, des, fuel, crystals);
		for (j = 1; j <= Num_races; j++)
		    if (isset(ownerbits,j))
			push_telegram(j,
				      (int)Stars[starnum]->governor[j-1],
				      telegram_buf);
	    }
	/* Leftovers for last player */
	for (; i <= Num_races; i++)
	    if (isset(ownerbits,i))
		break;
	if (i <= Num_races) { /* It should be */
	    res = stolenres - givenres;
	    des = stolendes - givendes;
	    fuel = stolenfuel - givenfuel;
	    crystals = stolencrystals - givencrystals;

	    planet->info[i-1].resource += res;
	    planet->info[i-1].destruct += des;
	    planet->info[i-1].fuel += fuel;
	    planet->info[i-1].crystals += crystals;
	    sprintf(telegram_buf, "%-14.14s %5d %5d %5d %5d",
		    races[i-1]->name, res, des, fuel, crystals);
	    sprintf(buf, "%-14.14s %5d %5d %5d %5d\n", "Total:",
		    stolenres, stolendes, stolenfuel, stolencrystals);
	    for (j = 1; j <= Num_races; j++)
		if (isset(ownerbits,j)) {
		    push_telegram(j, (int)Stars[starnum]->governor[j-1],
				  telegram_buf);
		    push_telegram(j, (int)Stars[starnum]->governor[j-1],
				  buf);
		}
	} else
	    push_telegram(1, 0, "Bug in stealing resources\n");
	/* Next: take all the loot away from the losers */
	for (i = 2; i <= Num_races; i++)
	    if (!isset(ownerbits,i)) {
		planet->info[i-1].resource = 0;
		planet->info[i-1].destruct = 0;
		planet->info[i-1].fuel = 0;
		planet->info[i-1].crystals = 0;
	    }
    }
}

double est_production(sectortype *s)
{
    return (races[s->owner-1]->metabolism*(double)s->eff*(double)s->eff/200.0);
}
