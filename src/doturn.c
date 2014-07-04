/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 * doturn -- does one turn.  
 */

#include <math.h>
#include <ctype.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "doturn.h"
#include "power.h"
#include "buffers.h"

FILE *fopen();
extern char *Commod[];
extern long Shipdata[NUMSTYPES][NUMABILS];

void do_turn(int);
int APadd(int, int, racetype *);
int governed(racetype *);
void fix_stability(startype *);
void do_reset(int);
void handle_victory(void);
void make_discoveries(racetype *);
#ifdef MARKET
void maintain(racetype *, int, int);
#endif
int attack_planet(shiptype *);
void output_ground_attacks(void);
int planet_points(planettype *);
#include "files_shl.p"
#include "doship.p"
#include "moveplanet.p"
#include "build.p"
#include "teleg_send.p"
#include "GB_server.p"
#include "shlmisc.p"
#include "rand.p"
#include "doplanet.p"

void do_turn(int update)
{
int star,i,j;
commodtype *c;
int dummy[2], temp;
double dist;
struct victstruct {
    int numsects;
    int shipcost;
    int shiptech;
    int morale;
    int res;
    int des;
    int fuel;
    int money;
} *victory;
  
/* make all 0 for first iteration of doplanet */
if(update) {
    bzero((char *)starpopns, sizeof(starpopns));
    bzero((char *)starnumships, sizeof(starnumships));
    bzero((char *)Sdatanumships, sizeof(Sdatanumships));
    bzero((char *)Stinfo, sizeof(Stinfo));
    bzero((char *)StarsInhab, sizeof(StarsInhab));
    bzero((char *)Power, sizeof(Power));
    bzero((char *) inhabited, sizeof(inhabited));
}

Num_ships = Numships();

for(i=1; i<=Num_ships; i++)
    domine(i,0);

ships = (shiptype **)malloc(sizeof(shiptype *) * (Num_ships + 1));
for(i=1; i<=Num_ships;i++)
    (void)getship(&ships[i], i);

/* get all stars and planets */
getsdata(&Sdata);
Planet_count = 0;
for (star=0; star<Sdata.numstars; star++) {
    getstar(&Stars[star], star);
    if(update)
	fix_stability(Stars[star]);     /* nova */

    for (i=0; i<Stars[star]->numplanets; i++) {
	getplanet(&planets[star][i], star, i);
	if(planets[star][i]->type != TYPE_ASTEROID)
	    Planet_count++;
	if(update)
	    moveplanet(star, planets[star][i], i);
	if(Stars[star]->pnames[i]=='\0')
	    sprintf(Stars[star]->pnames[i], "NULL-%d", i);
    }
    if(Stars[star]->name[0]=='\0')
	sprintf(Stars[star]->name, "NULL-%d", star);
}

VN_brain.Most_mad = 0;  /* not mad at anyone for starts */

for (i=1; i<=Num_races; i++) {
    		/* increase tech; change to something else */
    if(update) {
	reg int j;
	/* Reset controlled planet count */
	races[i-1]->controlled_planets = 0;
	races[i-1]->planet_points = 0;
	for(j=0; j<=MAXGOVERNORS; j++)
	    if(races[i-1]->governor[j].active) {
#ifdef MARKET
		races[i-1]->governor[j].maintain = 0;
		races[i-1]->governor[j].cost_market = 0;
		races[i-1]->governor[j].profit_market = 0;
#endif
		races[i-1]->governor[j].cost_tech = 0;
		races[i-1]->governor[j].income = 0;
	    }
	/* add VN program */
	VN_brain.Total_mad += Sdata.VN_hitlist[i-1];
	/* find out who they're most mad at */
	if (VN_brain.Most_mad > 0 &&
	    Sdata.VN_hitlist[VN_brain.Most_mad-1] <=
	    Sdata.VN_hitlist[i-1])
	    VN_brain.Most_mad = i;
    }
#ifdef VOTING
    /* Reset their vote for Update go. */
    races[i-1]->votes &= ~VOTE_UPDATE_GO;
#endif
}
output_ground_attacks();
#ifdef MARKET
if(update) {
    /* reset market */
    Num_commods = Numcommods();
    clr_commodfree();
    for(i=Num_commods; i>=1; i--) {
	getcommod(&c, i);
	if(!c->deliver) {
	    c->deliver = 1;
	    putcommod(c, i);
	    free(c);
	    continue;
	}
	if(c->owner && c->bidder &&
	   (races[c->bidder-1]->governor[c->bidder_gov].money >= c->bid)) {
	    races[c->bidder-1]->governor[c->bidder_gov].money -= c->bid;
	    races[c->owner-1]->governor[c->governor].money += c->bid;
	    temp=shipping_cost((int)c->star_to,
			       (int)c->star_from, &dist, (int)c->bid);
	    races[c->bidder-1]->governor[c->bidder_gov].cost_market +=
		c->bid+temp;
	    races[c->owner-1]->governor[c->governor].profit_market += c->bid;
	    maintain(races[c->bidder-1], (int)c->bidder_gov, temp);
	    switch(c->type) {
	      case RESOURCE:
		planets[c->star_to][c->planet_to]->info[c->bidder-1].resource += c->amount;
		break;
	      case FUEL:
		planets[c->star_to][c->planet_to]->info[c->bidder-1].fuel += c->amount;
		break;
	      case DESTRUCT:
		planets[c->star_to][c->planet_to]->info[c->bidder-1].destruct += c->amount;
		break;
	      case CRYSTAL:
		planets[c->star_to][c->planet_to]->info[c->bidder-1].crystals += c->amount;
		break;
	    }
	    sprintf(buf, "Lot %d purchased from %s [%d] at a cost of %d.\n   %d %s arrived at /%s/%s\n",
		    i, races[c->owner-1]->name, c->owner,
		    c->bid, c->amount, Commod[c->type],
		    Stars[c->star_to]->name,
		    Stars[c->star_to]->pnames[c->planet_to]);
	    push_telegram((int)c->bidder, (int)c->bidder_gov, buf);
	    sprintf(buf, "Lot %d (%d %s) sold to %s [%d] at a cost of %d.\n",
		    i, c->amount, Commod[c->type],
		    races[c->bidder-1]->name, c->bidder,
		    c->bid);
	    push_telegram((int)c->owner, (int)c->governor, buf);
	    c->owner = c->governor = 0;
	    c->bidder = c->bidder_gov = 0;
	} else {
	    c->bidder = c->bidder_gov = 0;
	    c->bid = 0;
	}
	if(!c->owner)
	    makecommoddead(i);
	putcommod(c, i);
	free(c);
    }
}
#endif

/* check ship masses - ownership */
for(i=1;i<=Num_ships; i++)
    if(ships[i]->alive) {
	domass(ships[i]);
	doown(ships[i]);
    }

  	/* do all ships one turn - do slower ships first */
for(j=0; j<=9; j++)
    for (i=1; i<=Num_ships; i++) {
	if(ships[i]->alive && ships[i]->speed==j) {
	    doship(ships[i], update);
	    if((ships[i]->type==STYPE_MISSILE) && !attack_planet(ships[i]))
		domissile(ships[i]);
	}
    }

#ifdef MARKET
/* do maintenance costs */
if(update)
    for (i=1; i<=Num_ships; i++)
	if(ships[i]->alive && Shipdata[ships[i]->type][ABIL_MAINTAIN]) {
	    if(ships[i]->popn)
		races[ships[i]->owner-1]->governor[ships[i]->governor].maintain +=
		    ships[i]->build_cost;
	    if(ships[i]->troops)
		races[ships[i]->owner-1]->governor[ships[i]->governor].maintain +=
		    UPDATE_TROOP_COST*ships[i]->troops;
	}
#endif
  
/* prepare dead ships for recycling */
clr_shipfree();
for(i=1; i<=Num_ships; i++)
    if(!ships[i]->alive)
	makeshipdead(i);	

  	/* erase next ship pointers - reset in insert_sh_... */
for (i=1; i<=Num_ships;i++) {
    ships[i]->nextship=0;
    ships[i]->ships = 0;
}
/* clear ship list for insertion */
Sdata.ships = 0;
for (star=0; star<Sdata.numstars; star++) {
    Stars[star]->ships = 0;
    for (i=0; i<Stars[star]->numplanets; i++)
	planets[star][i]->ships = 0;
}

/* insert ship into the list of wherever it might be */
for (i=Num_ships; i>=1; i--) {
    if(ships[i]->alive) {
	switch(ships[i]->whatorbits) {
	  case LEVEL_UNIV:
	    insert_sh_univ(&Sdata, ships[i]);
	    break;
	  case LEVEL_STAR:
	    insert_sh_star(Stars[ships[i]->storbits], ships[i]);
	    break;
	  case LEVEL_PLAN:
	    insert_sh_plan(planets[ships[i]->storbits][ships[i]->pnumorbits], ships[i]);
	    break;
	  case LEVEL_SHIP:
	    insert_sh_ship(ships[i], ships[ships[i]->destshipno]);
	    break;
	  default:
	    break;
	}
    }
}

/* put ABMs and surviving missiles here because ABMs need to have the missile
   in the shiplist of the target planet  Maarten */
for(i=1; i<=Num_ships; i++) /* ABMs defend planet */
    if((ships[i]->type==OTYPE_ABM) && ships[i]->alive)
	doabm(ships[i]);

for(i=1; i<=Num_ships; i++)
    if((ships[i]->type==STYPE_MISSILE) && ships[i]->alive
       && attack_planet(ships[i]))
	domissile(ships[i]);
  
for(i=Num_ships; i>=1; i--)
    putship(ships[i]);	

for (star=0; star<Sdata.numstars; star++) {
    for (i=0; i<Stars[star]->numplanets; i++) {
	/* store occupation for VPs */
	for (j=1; j<=Num_races; j++) {
	    if(planets[star][i]->info[j-1].numsectsowned) {
		setbit(inhabited[star], j);
		setbit(Stars[star]->inhabited, j);
	    }
	    if (planets[star][i]->type!=TYPE_ASTEROID &&
		(planets[star][i]->info[j-1].numsectsowned > 
		 planets[star][i]->Maxx * planets[star][i]->Maxy / 2))
		races[j-1]->controlled_planets++;

	    if(planets[star][i]->info[j-1].numsectsowned)
		races[j-1]->planet_points += planet_points(planets[star][i]);
	}
	if(update) {
	    if (doplanet(star, planets[star][i], i)) {
		/* save smap gotten & altered by doplanet
		   only if the planet is expl*/
		putsmap(Smap,planets[star][i]);
	    }
	}
	putplanet(planets[star][i], star, i);
    }
    /* do AP's for ea. player  */
    if(update)
	for (i=1; i<=Num_races; i++) {
	    if (starpopns[star][i-1])
		setbit(Stars[star]->inhabited,i);
	    else
		clrbit(Stars[star]->inhabited,i);

	    if (isset(Stars[star]->inhabited, i) ) {
		reg int APs;

		APs = Stars[star]->AP[i-1] +
		    APadd((int)starnumships[star][i-1],
			  (int)starpopns[star][i-1], races[i-1]);
		if(APs < LIMIT_APs)
		    Stars[star]->AP[i-1] = APs;
		else
		    Stars[star]->AP[i-1] = LIMIT_APs;
	    }
	    /* compute victory points for the block */
	    if(inhabited[star][0]+inhabited[star][1]) {
		dummy[0] = (Blocks[i-1].invite[0] & Blocks[i-1].pledge[0]);
		dummy[1] = (Blocks[i-1].invite[1] & Blocks[i-1].pledge[1]);
		Blocks[i-1].systems_owned += ((inhabited[star][0]|dummy[0])==dummy[0]) && ((inhabited[star][1]|dummy[1])==dummy[1]);
	    }
	}
    putstar(Stars[star], star);
}

/* add APs to sdata for ea. player */
if(update)
    for (i=1; i<=Num_races; i++) {
	Blocks[i-1].systems_owned = 0; /*recount systems owned*/
	if(governed(races[i-1])) {
	    reg int APs;
	    
	    APs = Sdata.AP[i-1] + races[i-1]->planet_points;
	    if(APs < LIMIT_APs)
		Sdata.AP[i-1] = APs;
	    else
		Sdata.AP[i-1] = LIMIT_APs;
	}
    }

putsdata(&Sdata);

/* here is where we do victory calculations. */
if (update) {
    victory = (struct victstruct *) malloc (Num_races *
					    sizeof (struct victstruct));
    for (i = 1; i <= Num_races; i++) {
	victory[i-1].numsects = 0;
	victory[i-1].shipcost = 0;
	victory[i-1].shiptech = 0;
	victory[i-1].morale = races[i-1]->morale;
	victory[i-1].res = 0;
	victory[i-1].des = 0;
	victory[i-1].fuel = 0;
	victory[i-1].money = races[i-1]->governor[0].money;
	for(j=1; j<=MAXGOVERNORS; j++)
	    if(races[i-1]->governor[j].active)
		victory[i-1].money += races[i-1]->governor[j].money;
    }

    for (star = 0; star < Sdata.numstars; star++) {
	/* do planets in the star next */
	for (i = 0; i < Stars[star]->numplanets; i++) {
	    for (j = 0; j < Num_races; j++) {
		if (!planets[star][i]->info[j].explored)
		    continue;
		victory[j].numsects += (int) planets[star][i]->info[j].numsectsowned;
		victory[j].res += (int) planets[star][i]->info[j].resource;
		victory[j].des += (int)planets[star][i]->info[j].destruct;
		victory[j].fuel += (int)planets[star][i]->info[j].fuel;
	    }
	} /* end of planet searchings */
    } /* end of star searchings */

    for (i = 1; i <= Num_ships; i++) {
	if (!ships[i]->alive)
	    continue;
	victory[ships[i]->owner-1].shipcost += ships[i]->build_cost;
	victory[ships[i]->owner-1].shiptech += ships[i]->tech;
	victory[ships[i]->owner-1].res += ships[i]->resource;
	victory[ships[i]->owner-1].des += ships[i]->destruct;
	victory[ships[i]->owner-1].fuel += ships[i]->fuel;
    }
    /* now that we have the info.. calculate the raw score */

    for (i = 0; i < Num_races; i++) {
	races[i]->victory_score = (VICT_SECT * (int)victory[i].numsects) +
	    (VICT_SHIP * ((int)victory[i].shipcost +
			  (VICT_TECH * (int)victory[i].shiptech))) +
			      (VICT_RES * ((int)victory[i].res +
					   (int)victory[i].des)) +
					       (VICT_FUEL * (int)victory[i].fuel) +
						   (VICT_MONEY * (int)victory[i].money);
	races[i]->victory_score /= VICT_DIVISOR;
	races[i]->victory_score
	    = (int)(morale_factor((double)victory[i].morale)*
		    races[i]->victory_score);
								      
    }
    free (victory);
} /* end of if (update) */

for (i=1; i<=Num_ships; i++) {
    putship(ships[i]);
    free(ships[i]);
}

if(update) {
    for (i=1; i<=Num_races; i++) {
	/* collective intelligence */
	if (races[i-1]->collective_iq) {
	  double x = ((2./3.14159265) * 
		      atan((double)Power[i-1].popn / MESO_POP_SCALE)) ;
	  races[i-1]->IQ = races[i-1]->IQ_limit * x * x;
	  }
	races[i-1]->tech += (double)(races[i-1]->IQ) / 100.0;
	races[i-1]->morale += Power[i-1].planets_owned;
	make_discoveries(races[i-1]);
	races[i-1]->turn += 1;
	if (races[i-1]->controlled_planets >=
	    Planet_count*VICTORY_PERCENT/100)
	    races[i-1]->victory_turns++;
	else
	    races[i-1]->victory_turns = 0;

	if (races[i-1]->controlled_planets >= Planet_count*VICTORY_PERCENT/200)
	    for (j = 1; j <= Num_races; j++)
		races[j-1]->translate[i-1] = 100;

	Blocks[i-1].VPs = 10 * Blocks[i-1].systems_owned;
#ifdef MARKET
	for(j=0; j<=MAXGOVERNORS; j++)
	    if(races[i-1]->governor[j].active)
		maintain(races[i-1], j, (int)races[i-1]->governor[j].maintain);
#endif
    }
    for (i = 1; i <= Num_races; i++)
	putrace(races[i-1]);
}

free(ships);

if(update) {
    compute_power_blocks();
    for(i=1;i<=Num_races; i++) {
	Power[i-1].money = 0;
	for(j=0; j<=MAXGOVERNORS; j++)
	    if(races[i-1]->governor[j].active)
		Power[i-1].money += races[i-1]->governor[j].money;
    }
    Putpower(Power);
    Putblock(Blocks);
}

for(j=1;j<=Num_races; j++) {
    if(update)
	notify_race(j, "Finished with update.\n");
    else
	notify_race(j, "Finished with movement segment.\n");
}
}

/* routine for number of AP's to add to each player in ea. system,scaled
    by amount of crew in their palace */

int APadd(int sh, int popn, racetype *race)
{
    int APs;

    APs = round_rand((double)sh/10.0 + 5.*log10(1.0+(double)popn));

    if(governed(race))
	return APs;
    else  /* dont have an active gov center */
	return round_rand((double)APs/20.);
}

int governed(racetype *race)
{
    return(race->Gov_ship && race->Gov_ship <= Num_ships &&
	ships[race->Gov_ship] != NULL && ships[race->Gov_ship]->alive &&
	ships[race->Gov_ship]->docked &&
	(ships[race->Gov_ship]->whatdest==LEVEL_PLAN ||
	 (ships[race->Gov_ship]->whatorbits==LEVEL_SHIP &&
	  ships[ships[race->Gov_ship]->destshipno]->type==STYPE_HABITAT &&
	  (ships[ships[race->Gov_ship]->destshipno]->whatorbits==LEVEL_PLAN ||
	   ships[ships[race->Gov_ship]->destshipno]->whatorbits==LEVEL_STAR))));
}

/* fix stability for stars */
void fix_stability(startype *s)
{
int a,i;

if (s->nova_stage > 0) {
    if (s->nova_stage > 14) {
	s->stability = 20;
	s->nova_stage = 0;
	sprintf(telegram_buf, "Notice\n");
	sprintf(buf,"\n  Scientists report that star %s\n",
		s->name);
	strcat(telegram_buf, buf);
	sprintf(buf, "is no longer undergoing nova.\n");
	strcat(telegram_buf, buf);
	for (i=1; i<=Num_races; i++)
	    push_telegram_race(i, telegram_buf);

	/* telegram everyone when nova over? */
    } else
	s->nova_stage++;
} else if (s->stability > 20) {
    a = int_rand(-1,3);
    /* nova just starting; notify everyone */
    if ( (s->stability + a) > 100) {
	s->stability = 100;
	s->nova_stage = 1;
	sprintf(telegram_buf, "***** BULLETIN! ******\n");
	sprintf(buf,"\n  Scientists report that star %s\n", s->name);
	strcat(telegram_buf, buf);
	sprintf(buf, "is undergoing nova.\n");
	strcat(telegram_buf, buf);
	for (i=1; i<=Num_races; i++)
	    push_telegram_race(i, telegram_buf);
    } else
	s->stability += a;
} else {
    a = int_rand(-1, 1 );
    if ( ((int)s->stability + a) < 0)
	s->stability = 0;
    else
	s->stability += a;
}
}

void do_reset(int time_reset)
{
int star,i;

Num_ships = Numships();

ships = (shiptype **)malloc(sizeof(shiptype *) * (Num_ships + 1));
for(i=1; i<=Num_ships;i++)
    (void)getship(&ships[i], i);

/* get all stars and planets */
getsdata(&Sdata);
for (star=0; star<Sdata.numstars; star++) {
    getstar(&Stars[star], star);
    for (i=0; i<Stars[star]->numplanets; i++)
	getplanet(&planets[star][i], star, i);
}
output_ground_attacks();

/* erase next ship pointers - reset in insert_sh_... */
for (i=1; i<=Num_ships;i++) {
    ships[i]->nextship=0;
    ships[i]->ships = 0;
}

/* clear ship list for insertion */
Sdata.ships = 0;
for (star=0; star<Sdata.numstars; star++) {
    Stars[star]->ships = 0;
    for (i=0; i<Stars[star]->numplanets; i++)
	planets[star][i]->ships = 0;	
}

/* insert ship into the list of wherever it might be */
for (i=Num_ships; i>=1; i--) {
    if(ships[i]->alive) {
	switch(ships[i]->whatorbits) {
	  case LEVEL_UNIV:
	    insert_sh_univ(&Sdata, ships[i]);
	    break;
	  case LEVEL_STAR:
	    insert_sh_star(Stars[ships[i]->storbits], ships[i]);
	    break;
	  case LEVEL_PLAN:
	    insert_sh_plan(planets[ships[i]->storbits][ships[i]->pnumorbits], ships[i]);
	    ships[i]->xpos = planets[ships[i]->storbits][ships[i]->pnumorbits]->xpos
		+Stars[ships[i]->storbits]->xpos;
	    ships[i]->ypos = planets[ships[i]->storbits][ships[i]->pnumorbits]->ypos
		+Stars[ships[i]->storbits]->ypos;
	    break;
	  case LEVEL_SHIP:
	    insert_sh_ship(ships[i], ships[ships[i]->destshipno]);
	    break;
	  default:
	    break;
	}
    }
}

/* check ship masses */
for(i=1;i<=Num_ships; i++)
    if(ships[i]->alive) {
	domass(ships[i]);
	doown(ships[i]);
    }

for (star=0; star<Sdata.numstars; star++) {
    if(!isascii(*Stars[star]->name))
	sprintf(Stars[star]->name, "%d", star);
    for (i=0; i<Stars[star]->numplanets; i++) {
	if(!isascii(*Stars[star]->pnames[i]))
	    sprintf(Stars[star]->pnames[i], "%d", i);
	putplanet(planets[star][i], star, i);
    }
    putstar(Stars[star], star);
}
putsdata(&Sdata);

for (i=1; i<=Num_ships; i++) {
    putship(ships[i]);
    free(ships[i]);
}
free(ships);

for (i=1; i<=Num_races; i++) {
    putrace(races[i-1]);
    notify_race(i, "Finished with reset.\n");
}
}

#define BIG_WINNER 1
#define LITTLE_WINNER 2

void handle_victory(void)
{
int i, j;
int game_over = 0;
int win_category[64];

#ifndef VICTORY
return;
#endif
  
for (i = 1; i <= Num_races; i++) {
    win_category[i-1] = 0;
    if (races[i-1]->controlled_planets >=
	Planet_count*VICTORY_PERCENT/100) {
	win_category[i-1] = LITTLE_WINNER;
    }
    if (races[i-1]->victory_turns >= VICTORY_UPDATES) {
	game_over++;
	win_category[i-1] = BIG_WINNER;
    } 
}
if (game_over) {
    for (i = 1; i <= Num_races; i++) {
	sprintf(telegram_buf, "*** Attention ***");
	push_telegram_race(i, telegram_buf);
	sprintf(telegram_buf,
		"This game of Galactic Bloodshed is now *over*");
	push_telegram_race(i, telegram_buf);
	sprintf(telegram_buf, "The big winner%s",
		(game_over == 1)?" is":"s are");
	push_telegram_race(i, telegram_buf);
	for (j = 1; j <= Num_races; j++)
	    if (win_category[j-1] == BIG_WINNER) {
		sprintf(telegram_buf, "*** [%2d] %-30.30s ***",
			j, races[j-1]->name);
		push_telegram_race(i, telegram_buf);
	    }
	sprintf(telegram_buf, "Lesser winners:");
	push_telegram_race(i, telegram_buf);
	for (j = 1; j <= Num_races; j++)
	    if (win_category[j-1] == LITTLE_WINNER) {
		sprintf(telegram_buf, "+++ [%2d] %-30.30s +++",	j, races[j-1]->name);
		push_telegram_race(i, telegram_buf);
	    }
    }
}
}

void make_discoveries(racetype *r)
{
    /* would be nicer to do this with a loop of course - but it's late */
    if(!Hyper_drive(r) && r->tech >= TECH_HYPER_DRIVE) {
	push_telegram_race(r->Playernum,
			   "You have discovered HYPERDRIVE technology.\n");
	r->discoveries[D_HYPER_DRIVE]=1;
    }
    if(!Laser(r) && r->tech >= TECH_LASER) {
	push_telegram_race(r->Playernum,
			   "You have discovered LASER technology.\n");
	r->discoveries[D_LASER]=1;
    }
    if(!Cew(r) && r->tech >= TECH_CEW) {
	push_telegram_race(r->Playernum,
			   "You have discovered CEW technology.\n");
	r->discoveries[D_CEW]=1;
    }
    if(!Vn(r) && r->tech >= TECH_VN) {
	push_telegram_race(r->Playernum,
			   "You have discovered VN technology.\n");
	r->discoveries[D_VN]=1;
    }
    if(!Tractor_beam(r) && r->tech >= TECH_TRACTOR_BEAM) {
	push_telegram_race(r->Playernum,
			   "You have discovered TRACTOR BEAM technology.\n");
	r->discoveries[D_TRACTOR_BEAM]=1;
    }
    if(!Transporter(r) && r->tech >= TECH_TRANSPORTER) {
	push_telegram_race(r->Playernum,
			   "You have discovered TRANSPORTER technology.\n");
	r->discoveries[D_TRANSPORTER]=1;
    }
    if(!Avpm(r) && r->tech >= TECH_AVPM) {
	push_telegram_race(r->Playernum,
			   "You have discovered AVPM technology.\n");
	r->discoveries[D_AVPM]=1;
    }
    if(!Cloak(r) && r->tech >= TECH_CLOAK) {
	push_telegram_race(r->Playernum,
		      "You have discovered CLOAK technology.\n");
	r->discoveries[D_CLOAK]=1;
    }
    if(!Wormhole(r) && r->tech >= TECH_WORMHOLE) {
	push_telegram_race(r->Playernum,
			   "You have discovered WORMHOLE technology.\n");
	r->discoveries[D_WORMHOLE]=1;
    }
    if(!Crystal(r) && r->tech >= TECH_CRYSTAL) {
	push_telegram_race(r->Playernum,
			   "You have discovered CRYSTAL technology.\n");
	r->discoveries[D_CRYSTAL]=1;
    }
}

#ifdef MARKET
void maintain(racetype *r, int gov, int amount)
{
    if(r->governor[gov].money >= amount)
	r->governor[gov].money -= amount;
    else {
	r->morale -= (amount - r->governor[gov].money)/10;
	r->governor[gov].money = 0;
    }
}
#endif

int attack_planet(shiptype *ship)
{
    if(ship->whatdest==LEVEL_PLAN)
	return 1;
    else
	return 0;
}

void output_ground_attacks(void)
{
    int star, i, j;
    
    for(star=0; star<Sdata.numstars; star++)
	for(i=1; i<=Num_races; i++)
	    for(j=1; j<=Num_races; j++)
		if(ground_assaults[i-1][j-1][star]) {
     		    sprintf(buf, "%s: %s [%d] assaults %s [%d] %d times.\n",
			    Stars[star]->name,
			    races[i-1]->name, i, races[j-1]->name, j,
			    ground_assaults[i-1][j-1][star]);
		    post(buf, COMBAT);
		    ground_assaults[i-1][j-1][star] = 0;
		}
}

int planet_points(planettype *p)
{
    switch(p->type) {
      case TYPE_ASTEROID: return ASTEROID_POINTS;
      case TYPE_EARTH: return EARTH_POINTS;
      case TYPE_MARS: return MARS_POINTS;
      case TYPE_ICEBALL: return ICEBALL_POINTS;
      case TYPE_GASGIANT: return GASGIANT_POINTS;
      case TYPE_WATER: return WATER_POINTS;
      case TYPE_FOREST: return FOREST_POINTS;
      case TYPE_DESERT: return DESERT_POINTS;
      default: return 0;
    }
}
    
