/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  load.c -- load/unload stuff
 */
#include <signal.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

extern char Dessymbols[];
extern char *Desnames[];
extern int Defensedata[];
char buff[128], bufr[128], bufd[128], bufc[128], bufx[128], bufm[128];

void load(int, int, int, int);
void jettison(int, int, int);
int jettison_check(int, int, int, int);
void dump(int, int, int);
void transfer(int, int, int);
void mount(int, int, int, int);
void use_fuel(shiptype *, double);
void use_destruct(shiptype *, int);
void use_resource(shiptype *, int);
void use_popn(shiptype *, int, double);
void rcv_fuel(shiptype *, double);
void rcv_resource(shiptype *, int);
void rcv_destruct(shiptype *, int);
void rcv_popn(shiptype *, int, double);
void rcv_troops(shiptype *, int, double);
void do_transporter(racetype *, int, shiptype *);
int landed_on(shiptype *, int);
void unload_onto_alien_sector(int, int, planettype *, shiptype *,
			      sectortype *, int, int);
#include "getplace.p"
#include "GB_server.p"
#include "shlmisc.p"
#include "files_shl.p"
#include "fire.p"
#include "max.p"
#include "move.p"
#include "land.p"
#include "rand.p"

void load(int Playernum, int Governor, int APcount, int mode)
{
char commod;
unsigned char sh=0,diff=0;
int proc, lolim, uplim, amt;
int transfercrew;
shiptype *s,*s2;
planettype *p;
sectortype *sect;
racetype *Race;
int shipno, nextshipno;

if(argn < 2) {
    notify(Playernum, Governor, "Load what?\n");
    return;
}

nextshipno = start_shiplist(Playernum, Governor, args[1]);

while(shipno = do_shiplist(&s, &nextshipno))
    if(in_list(Playernum, args[1], s, &nextshipno) &&
       authorized(Governor, s)) {
	if (s->owner!=Playernum || !s->alive) {
	    free(s);
	    continue;
	}
	if (!s->active) {
	    sprintf(buf,"%s is irradiated and inactive.\n", Ship(s));
	    notify(Playernum, Governor, buf);
	    free(s);
	    continue;
	}
	if (s->whatorbits==LEVEL_UNIV) {
	    if (!enufAP(Playernum,Governor,Sdata.AP[Playernum-1], APcount)) {
		free(s);
		continue;
	    }
	} else
	    if(!enufAP(Playernum,Governor,Stars[s->storbits]->AP[Playernum-1], APcount))
		continue;
	proc = 0;
	if (!s->docked) {
	    sprintf(buf,"%s is not landed or docked.\n", Ship(s));
	    notify(Playernum, Governor, buf);
	    free(s);
	    continue;
	} else {	/* ship has a recipient */
	    if (s->whatdest==LEVEL_PLAN) {
		sprintf(buf,"%s at %d,%d\n", Ship(s), s->land_x,s->land_y);
		notify(Playernum, Governor, buf);
		if(s->storbits != Dir[Playernum-1][Governor].snum ||
		   s->pnumorbits != Dir[Playernum-1][Governor].pnum) {
		    notify(Playernum, Governor, "Change scope to the planet this ship is landed on.\n");
		    free(s);
		    continue;
		}
	    } else {	/* ship is docked */
		if(!s->destshipno) {
		    sprintf(buf, "%s is not docked.\n", Ship(s));
		    free(s);
		    continue;
		}
		if(!getship(&s2, (int)s->destshipno)) {
		    notify(Playernum, Governor, "Destination ship is bogus.\n");
		    free(s); continue;
		}
		if (!s2->alive ||
		    !(s->whatorbits==LEVEL_SHIP || s2->destshipno==shipno)) {
		    /* the ship it was docked with died or
		       undocked with it or something. */
		    s->docked = 0;
		    s->whatdest = LEVEL_UNIV;
		    putship(s);
		    sprintf(buf,"%s is not docked.\n",Ship(s2));
		    notify(Playernum, Governor, buf);
		    free(s);
		    free(s2);
		    continue;
		}
		if(overloaded(s2) && s2->whatorbits==LEVEL_SHIP) {
		    sprintf(buf, "%s is overloaded!\n", Ship(s2));
		    notify(Playernum, Governor, buf);
		    free(s); free(s2); continue;
		}
		sprintf(buf,"%s docked with %s\n", Ship(s), Ship(s2));
		notify(Playernum, Governor, buf);
		sh = 1;
		if (s2->owner!=Playernum) {
		    sprintf(buf,"Player %d owns that ship.\n", s2->owner);
		    notify(Playernum, Governor, buf);
		    diff = 1;
		}
	    }
	}

	commod = args[2][0];
	if(argn>3)
	    amt = atoi(args[3]);
	else
	    amt = 0;

	if(mode) amt = -amt;  /* unload */

	if(amt<0 && s->type==OTYPE_VN) {
	   notify(Playernum, Governor, "You can't unload VNs.\n");
	   free(s);
	   if(sh) free(s2);
	   continue;
	}

	if (!sh)
	    getplanet(&p,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);

	if (!sh && (commod=='c' || commod=='m'))
	    getsector(&sect, p, (int)s->land_x, (int)s->land_y);

	switch (commod) {
	  case 'x':
	  case '&':
	    if (sh) {
		uplim = diff ? 0 : MIN(s2->crystals, Max_crystals(s) - s->crystals);
		lolim = diff ? 0 : - MIN(s->crystals, Max_crystals(s2) - s2->crystals);
	    } else {
		uplim = MIN(p->info[Playernum-1].crystals, Max_crystals(s) - s->crystals);
		lolim = -s->crystals;
	    }
	    proc = 1;
	    break;
	  case 'c':
	    if (sh) {
		uplim = diff ? 0 : MIN(s2->popn, Max_crew(s) - s->popn);
		lolim = diff ? 0 : - MIN(s->popn, Max_crew(s2) - s2->popn);
	    } else {
		uplim = MIN(sect->popn, Max_crew(s) - s->popn);
		lolim = -s->popn;
	    }
	    proc = 1;
	    break;
	  case 'm':
	    if (sh) {
		uplim = diff ? 0 : MIN(s2->troops, Max_mil(s) - s->troops);
		lolim = diff ? 0 : - MIN(s->troops, Max_mil(s2) - s2->troops);
	    } else {
		uplim = MIN(sect->troops, Max_mil(s) - s->troops);
		lolim = -s->troops;
	    }
	    proc = 1;
	    break;
	  case 'd':
	    if (sh) {
		uplim = diff ? 0 : MIN(s2->destruct, Max_destruct(s) - s->destruct);
		lolim = -MIN(s->destruct, Max_destruct(s2) - s2->destruct);
	    } else {
		uplim = MIN(p->info[Playernum-1].destruct, Max_destruct(s) - s->destruct);
		lolim = -s->destruct;
	    }
	    proc = 1;
	    break;
	  case 'f':
	    if (sh) {
		uplim = diff ? 0 : MIN((int)s2->fuel, (int)Max_fuel(s)-(int)s->fuel);
		lolim = -MIN((int)s->fuel, (int)Max_fuel(s2)-(int)s2->fuel);
	    } else {
		uplim = MIN((int)p->info[Playernum-1].fuel, (int)Max_fuel(s) - (int)s->fuel);
		lolim = -(int)s->fuel;
	    }
	    proc = 1;
	    break;
	  case 'r':
	    if (sh) {
		if (s->type==STYPE_SHUTTLE && s->whatorbits!=LEVEL_SHIP)
		    uplim = diff ? 0 : s2->resource;
		else
		    uplim = diff ? 0 : MIN(s2->resource, Max_resource(s) - s->resource);
		if (s2->type==STYPE_SHUTTLE && s->whatorbits!=LEVEL_SHIP)
		    lolim = -s->resource;
		else
		    lolim = -MIN(s->resource, Max_resource(s2) - s2->resource);
	    } else {
		uplim = MIN(p->info[Playernum-1].resource, Max_resource(s) - s->resource);
		lolim = -s->resource;
	    }
	    proc = 1;
	    break;
	  default:
	    notify(Playernum, Governor, "No such commodity valid.\n");
	    if(sh)
		free(s2);
	    else
		free(p);
	    free(s);
	    continue;
	}

	if(amt<lolim || amt>uplim) {
	    sprintf(buf, "you can only transfer between %d and %d.\n", lolim, uplim);
	    notify(Playernum, Governor, buf);

	    if(sh)	
		free(s2);
	    else
		free(p);
	    free(s);
	    continue;
	}

	Race = races[Playernum-1];

	if(amt==0)
	    amt = (mode ? lolim : uplim);

	switch (commod) {
	  case 'c':
	    if (sh) {
		s2->popn -= amt;
		if(!landed_on(s, sh))
		    s2->mass -= amt*Race->mass;
		transfercrew = 1;
	    } else if(sect->owner && sect->owner != Playernum) {
		sprintf(buf,"That sector is already occupied by another player!\n");
		notify(Playernum, Governor, buf);
/* fight a land battle */
		unload_onto_alien_sector(Playernum, Governor, p, s, sect, CIV, -amt);
		putship(s);
		putsector(sect, p, (int)s->land_x, (int)s->land_y);
		putplanet(p,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);
		free(s); free(sect); free(p);
		return;
	    } else {
		transfercrew = 1;
		if (!sect->popn && !sect->troops && amt<0) {
		    p->info[Playernum-1].numsectsowned++;
		    p->info[Playernum-1].mob_points += sect->mobilization;
		    sect->owner = Playernum;
		    sprintf(buf,"sector %d,%d COLONIZED.\n",
			    s->land_x,s->land_y);
		    notify(Playernum, Governor, buf);
		}
		sect->popn -= amt;
		p->popn -= amt;
		p->info[Playernum-1].popn -= amt;
		if (!sect->popn && !sect->troops) {
		    p->info[Playernum-1].numsectsowned--;
		    p->info[Playernum-1].mob_points -= sect->mobilization;
		    sect->owner = 0;
		    sprintf(buf,"sector %d,%d evacuated.\n",
			    s->land_x,s->land_y);
		    notify(Playernum, Governor, buf);
		}
	    }
	    if(transfercrew) {
		s->popn += amt;
		s->mass += amt*Race->mass;
		sprintf(buf,"crew complement of %s is now %u.\n",
			Ship(s), s->popn);
		notify(Playernum, Governor, buf);
	    }
	    break;
	  case 'm':
	    if (sh) {
		s2->troops -= amt;
		if(!landed_on(s, sh))
		    s2->mass -= amt*Race->mass;
		transfercrew = 1;
	    } else if(sect->owner && sect->owner != Playernum) {
		sprintf(buf,"That sector is already occupied by another player!\n");
		notify(Playernum, Governor, buf);
		unload_onto_alien_sector(Playernum, Governor, p, s, sect, MIL, -amt);
		putship(s);
		putsector(sect, p, (int)s->land_x, (int)s->land_y);
		putplanet(p,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);
		free(s); free(sect); free(p);
		return;
	    } else {
		transfercrew = 1;
		if (!(sect->popn+sect->troops) && amt<0) {
		    p->info[Playernum-1].numsectsowned++;
		    p->info[Playernum-1].mob_points += sect->mobilization;
		    sect->owner = Playernum;
		    sprintf(buf,"sector %d,%d OCCUPIED.\n",
			    s->land_x,s->land_y);
		    notify(Playernum, Governor, buf);
		}
		sect->troops -= amt;
		p->troops -= amt;
		p->info[Playernum-1].troops -= amt;
		if (!(sect->troops+sect->popn)) {
		    p->info[Playernum-1].numsectsowned--;
		    p->info[Playernum-1].mob_points -= sect->mobilization;
		    sect->owner = 0;
		    sprintf(buf,"sector %d,%d evacuated.\n",
			    s->land_x,s->land_y);
		    notify(Playernum, Governor, buf);
		}
	    }
	    if(transfercrew) {
		s->troops += amt;
		s->mass += amt*Race->mass;
		sprintf(buf,"troop complement of %s is now %u.\n",
			Ship(s), s->troops);
		notify(Playernum, Governor, buf);
	    }
	    break;
	  case 'd': 
	    if (sh) {
		s2->destruct -= amt;
		if(!landed_on(s, sh))
		    s2->mass -= amt*MASS_DESTRUCT;
	    } else
		p->info[Playernum-1].destruct -= amt;

	    s->destruct += amt;
	    s->mass += amt*MASS_DESTRUCT;
	    sprintf(buf, "%d destruct transferred.\n", amt);
	    notify(Playernum, Governor, buf);
	    if (!Max_crew(s)) {
		sprintf(buf,"\n%s ", Ship(s));
		notify(Playernum, Governor, buf);
		if (s->destruct) {
		    sprintf(buf,"now boobytrapped.\n");
		} else {
		    sprintf(buf,"no longer boobytrapped.\n");
		}
		notify(Playernum, Governor, buf);
	    }
	    break;
	  case 'x':
	    if (sh) {
		s2->crystals -= amt;
	    } else
		p->info[Playernum-1].crystals -= amt;
	    s->crystals += amt;
	    sprintf(buf, "%d crystal(s) transferred.\n", amt);
	    notify(Playernum, Governor, buf);
	    break;
	  case 'f':
	    if (sh) {
		s2->fuel -= (double)amt;
		if(!landed_on(s, sh))
		    s2->mass -= (double)amt*MASS_FUEL;
	    } else
		p->info[Playernum-1].fuel -= amt;
	    rcv_fuel(s, (double)amt);
	    sprintf(buf, "%d fuel transferred.\n", amt);
	    notify(Playernum, Governor, buf);
	    break;
	  case 'r':
	    if (sh) {
		s2->resource -= amt;
		if(!landed_on(s, sh))
		    s2->mass -= amt*MASS_RESOURCE;
	    } else
		p->info[Playernum-1].resource -= amt;
	    rcv_resource(s, amt);
	    sprintf(buf, "%d resources transferred.\n", amt);
	    notify(Playernum, Governor, buf);
	    break;
	  default:
	    notify(Playernum, Governor, "No such commodity.\n");

	    if(sh)
		free(s2);
	    else
		free(p);
	    free(s);
	    continue;
	}

	if (sh) {
    /* ship to ship transfer */
	    buff[0] = bufr[0] = bufd[0] = bufc[0] = '\0';
	    switch (commod) {
	      case 'r':
		sprintf(buf,"%d resources transferred.\n", amt);
		notify(Playernum, Governor, buf);
		sprintf(bufr, "%d Resources\n",amt);
		break;
	      case 'f':
		sprintf(buf,"%d fuel transferred.\n", amt);
		notify(Playernum, Governor, buf);
		sprintf(buff, "%d Fuel\n",amt);
		break;
	      case 'd':
		sprintf(buf,"%d destruct transferred.\n", amt);
		notify(Playernum, Governor, buf);
		sprintf(bufd, "%d Destruct\n",amt);
		break;
	      case 'x':
	      case '&':
		sprintf(buf,"%d crystals transferred.\n", amt);
		notify(Playernum, Governor, buf);
		sprintf(bufd, "%d Crystal(s)\n",amt);
		break;
	      case 'c':
		sprintf(buf,"%d popn transferred.\n", amt);
		notify(Playernum, Governor, buf);
		sprintf(bufc, "%d %s\n", amt,
			Race->Metamorph?"tons of biomass" : "population");
		break;
	      case 'm':
		sprintf(buf,"%d military transferred.\n", amt);
		notify(Playernum, Governor, buf);
		sprintf(bufm, "%d %s\n", amt,
			Race->Metamorph?"tons of biomass" : "population");
		break;
	      default:
		break;
	    }
	    putship(s2);
	    free(s2);
	} else {
	    if (commod=='c' || commod=='m') {
		putsector(sect, p, (int)s->land_x, (int)s->land_y);
		free(sect);
	    }
	    putplanet(p,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);
	    free(p);
	}

	/* do transporting here */
	if (s->type == OTYPE_TRANSDEV && s->special.transport.target && s->on)
	    do_transporter(Race, Governor, s);

	putship(s);
	free(s);
    } else
	free(s);	/* make sure you do this! */
}

void jettison(int Playernum, int Governor, int APcount)
{
  int Mod = 0;
  int shipno, nextshipno;
  int amt;
  char commod;
  shiptype *s;
  racetype *Race;

  if (argn < 2) {
    notify(Playernum, Governor, "Jettison what?\n");
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, args[1]);

  while(shipno = do_shiplist(&s, &nextshipno))
      if(in_list(Playernum, args[1], s, &nextshipno) &&
	 authorized(Governor, s)) {
	  if (s->owner!=Playernum || !s->alive) {
	      free(s);
	      continue;
	  }
	  if (landed(s)) {
	      notify(Playernum, Governor, "Ship is landed, cannot jettison.\n");
	      free(s);
	      continue;
	  }
	  if (!s->active) {
	      sprintf(buf,"%s is irradiated and inactive.\n", Ship(s));
	      notify(Playernum, Governor, buf);
	      free(s);
	      continue;
	  }
	  if (s->whatorbits==LEVEL_UNIV) {
	      if (!enufAP(Playernum,Governor,Sdata.AP[Playernum-1], APcount)) {
		  free(s);
		  continue;
	      }
	  } else if (!enufAP(Playernum,Governor,Stars[s->storbits]->AP[Playernum-1], APcount)) {
	      free(s);
	      continue;
	  }

	  if (argn > 3) amt = atoi(args[3]);
	  else amt = 0;

	  Race = races[Playernum-1];

	  commod = args[2][0];
	  switch (commod) {
	    case 'x':
	      if ( (amt = jettison_check(Playernum, Governor, amt, (int)(s->crystals))) > 0) {
		  s->crystals -= amt;
		  sprintf(buf, "%d crystal%s jettisoned.\n", amt, (amt == 1)?"":"s");
		  notify(Playernum, Governor, buf);
		  Mod = 1;
	      }
	      break;
	    case 'c':
	      if ( (amt = jettison_check(Playernum, Governor, amt, (int)(s->popn))) > 0) {
		  s->popn -= amt;
		  s->mass -= amt*Race->mass;
		  sprintf(buf,"%d crew %s into deep space.\n",
			  amt, (amt == 1)?"hurls itself":"hurl themselves");
		  notify(Playernum, Governor, buf);
		  sprintf(buf, "Complement of %s is now %u.\n", Ship(s), s->popn);
		  notify(Playernum, Governor, buf);
		  Mod = 1;
	      }
	      break;
	    case 'm':
	      if ( (amt = jettison_check(Playernum, Governor, amt, (int)(s->troops))) > 0) {
		  sprintf(buf,"%d military %s into deep space.\n",
			  amt, (amt == 1)?"hurls itself":"hurl themselves");
		  notify(Playernum, Governor, buf);
		  sprintf(buf, "Complement of ship #%d is now %u.\n",
			  shipno, s->troops-amt);
		  notify(Playernum, Governor, buf);
		  s->troops -= amt;
		  s->mass -= amt*Race->mass;
		  Mod = 1;
	      }
	      break;
	    case 'd':
	      if ( (amt = jettison_check(Playernum, Governor, amt, (int)(s->destruct))) > 0) {
		  use_destruct(s, amt);
		  sprintf(buf, "%d destruct jettisoned.\n", amt);
		  notify(Playernum, Governor, buf);
		  if (!Max_crew(s)) {
		      sprintf(buf,"\n%s ", Ship(s));
		      notify(Playernum, Governor, buf);
		      if (s->destruct) {
			  notify(Playernum, Governor,"still boobytrapped.\n");
		      } else {
			  notify(Playernum, Governor,"no longer boobytrapped.\n");
		      }
		  }
		  Mod = 1;
	      }
	      break;
	    case 'f':
	      if ( (amt = jettison_check(Playernum, Governor, amt, (int)(s->fuel))) > 0) {
		  use_fuel(s, (double)amt);
		  sprintf(buf, "%d fuel jettisoned.\n", amt);
		  notify(Playernum, Governor, buf);
		  Mod = 1;
	      }
	      break;
	    case 'r':
	      if ( (amt = jettison_check(Playernum, Governor, amt, (int)(s->resource))) > 0) {
		  use_resource(s, amt);
		  sprintf(buf, "%d resources jettisoned.\n", amt);
		  notify(Playernum, Governor, buf);
		  Mod = 1;
	      }
	      break;
	    default:
	      notify(Playernum, Governor, "No such commodity valid.\n");
	      return;
	  }
	  if (Mod)
	      putship(s);
	  free(s);
      } else
	  free(s);
}

int jettison_check(int Playernum, int Governor, int amt, int max)
{
  if (amt == 0) amt = max;
  if (amt < 0) {
    notify(Playernum, Governor, "Nice try.\n");
    return -1;
  } else if (amt > max) {
    sprintf(buf, "You can jettison at most %d\n", max);
    notify(Playernum, Governor, buf);
    return -1;
  }
  return amt;
}

void dump(int Playernum, int Governor, int APcount)
{
  int player, star, i, j;
  racetype *Race, *r;
  placetype where;

  if (!enufAP(Playernum,Governor,Stars[Dir[Playernum-1][Governor].snum]->AP[Playernum-1], APcount))
    return;

  if(!(player=GetPlayer(args[1]))) {
    sprintf(buf,"No such player.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  r = races[player-1];
  
  if(r->Guest) {
    notify(Playernum, Governor, "Cheater!\n");
    return;
  }

  /* transfer all planet and star knowledge to the player */
  /* get all stars and planets */
  Race = races[Playernum-1];
  if(Race->Guest) {
    notify(Playernum, Governor, "Cheater!\n");
    return;
  }
  if(Governor) {
      notify(Playernum, Governor, "Only leaders are allowed to use dump.\n");
      return;
  }
  getsdata(&Sdata);

  if(argn < 3) {
    for (star=0; star<Sdata.numstars; star++) {
      getstar(&Stars[star], star);

      if(isset(Stars[star]->explored, Playernum)) {
        setbit(Stars[star]->explored, player);

        for (i=0; i<Stars[star]->numplanets; i++) {
          getplanet(&planets[star][i],star,i);
          if(planets[star][i]->info[Playernum-1].explored) {
            planets[star][i]->info[player-1].explored = 1;
            putplanet(planets[star][i],star,i);
          }
        }
        putstar(Stars[star], star);
      }
    }
  } else {  /* list of places given */
    for(i=2; i<argn; i++) {
      where = Getplace(Playernum, Governor, args[i], 1);
      if(!where.err && where.level!=LEVEL_UNIV &&
          where.level!=LEVEL_SHIP) {
        star = where.snum;
        getstar(&Stars[star], star);

        if(isset(Stars[star]->explored, Playernum)) {
          setbit(Stars[star]->explored, player);

          for (j=0; j<Stars[star]->numplanets; j++) {
            getplanet(&planets[star][j],star,j);
            if(planets[star][j]->info[Playernum-1].explored) {
              planets[star][j]->info[player-1].explored = 1;
              putplanet(planets[star][j],star,j);
            }
          }
          putstar(Stars[star], star);
        }
      }
    }
  }

  deductAPs(Playernum, Governor, APcount, Dir[Playernum-1][Governor].snum, 0);

  sprintf(buf, "%s [%d] has given you exploration data.\n", Race->name, Playernum);
  warn_race(player, buf);
  notify(Playernum, Governor, "Exploration Data transferred.\n");
}

void transfer(int Playernum, int Governor, int APcount)
{
  int Mod = 0, player, give;
  planettype *planet;
  char commod=0;
  racetype *r;

  if (Dir[Playernum-1][Governor].level != LEVEL_PLAN) {
    sprintf(buf,"You need to be in planet scope to do this.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  if (!enufAP(Playernum,Governor,Stars[Dir[Playernum-1][Governor].snum]->AP[Playernum-1], APcount))
    return;

  if(!(player=GetPlayer(args[1]))) {
    sprintf(buf,"No such player.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  r = races[player-1];
  
  getplanet(&planet, Dir[Playernum-1][Governor].snum, Dir[Playernum-1][Governor].pnum);

  sscanf(args[2],"%c",&commod);
  give = atoi(args[3]);

  if(give < 0) {
    notify(Playernum, Governor, "You must specify a positive amount.\n");
    free(planet);
    return;
  }
  sprintf(temp, "%s/%s:", Stars[Dir[Playernum-1][Governor].snum]->name,
	  Stars[Dir[Playernum-1][Governor].snum]->pnames[Dir[Playernum-1][Governor].pnum]);
  switch(commod) {
  case 'r' :
    if(give > planet->info[Playernum-1].resource)
    {
      sprintf(buf,"You don't have %d on this planet.\n",give);
      notify(Playernum, Governor, buf);
    } else {
      planet->info[Playernum-1].resource -= give;
      planet->info[player-1].resource += give;
      sprintf(buf,"%s %d resources transferred from player %d to player #%d\n",
          temp, give, Playernum, player);
      notify(Playernum, Governor, buf);
      warn_race(player, buf);
    }
    break;
  case 'x' :
  case '&' :
    if(give > planet->info[Playernum-1].crystals)
    {
      sprintf(buf,"You don't have %d on this planet.\n",give);
      notify(Playernum, Governor, buf);
    } else {
      planet->info[Playernum-1].crystals -= give;
      planet->info[player-1].crystals += give;
      sprintf(buf,"%s %d crystal(s) transferred from player %d to player #%d\n",
	      temp,give,Playernum,player);
      notify(Playernum, Governor, buf);
      warn_race(player, buf);
    }
    break;
  case 'f' :
    if(give > planet->info[Playernum-1].fuel)
    {
      sprintf(buf,"You don't have %d fuel on this planet.\n",give);
      notify(Playernum, Governor, buf);
    } else {
      planet->info[Playernum-1].fuel -= give;
      planet->info[player-1].fuel += give;
      sprintf(buf,"%s %d fuel transferred from player %d to player #%d\n",
	      temp,give,Playernum,player);
      notify(Playernum, Governor, buf);
      warn_race(player, buf);
    }
    break;
  case 'd' :
    if(give > planet->info[Playernum-1].destruct)
    {
      sprintf(buf,"You don't have %d destruct on this planet.\n",give);
      notify(Playernum, Governor, buf);
    } else {
      planet->info[Playernum-1].destruct -= give;
      planet->info[player-1].destruct += give;
      sprintf(buf,"%s %d destruct transferred from player %d to player #%d\n",
	      temp,give,Playernum,player);
      notify(Playernum, Governor, buf);
      warn_race(player, buf);
    }
    break;
  default :
    sprintf(buf,"What?\n");
    notify(Playernum, Governor, buf);
  }

  putplanet(planet,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);

  free(planet);
  Mod = 1;

  deductAPs(Playernum, Governor, APcount, Dir[Playernum-1][Governor].snum, 0);

}

void mount(int Playernum, int Governor, int APcount, int mnt)
{
  shiptype *ship;
  int shipno, nextshipno;

  nextshipno = start_shiplist(Playernum, Governor, args[1]);
  while(shipno = do_shiplist(&ship, &nextshipno))
      if(in_list(Playernum, args[1], ship, &nextshipno) &&
	 authorized(Governor, ship)) {
	  if(!ship->mount) {
	      notify(Playernum, Governor, "This ship is not equipped with a crystal mount.\n");
	      free(ship);
	      continue;
	  }
	  if(ship->mounted && mnt) {
	      notify(Playernum, Governor, "You already have a crystal mounted.\n");
	      free(ship);
	      continue;
	  } else if(!ship->mounted && !mnt) {
	      notify(Playernum, Governor, "You don't have a crystal mounted.\n");
	      free(ship);
	      continue;
	  } else if(!ship->mounted && mnt) {
	      if(!ship->crystals) {
		  notify(Playernum, Governor, "You have no crystals on board.\n");
		  free(ship);
		  continue;
	      }
	      ship->mounted=1;
	      ship->crystals--;
	      notify(Playernum, Governor, "Mounted.\n");
	  } else if(ship->mounted && !mnt) {
	      if(ship->crystals==Max_crystals(ship)) {
		  notify(Playernum, Governor, "You can't dismount the crystal. Max allowed already on board.\n");
		  free(ship);
		  continue;
	      }
	      ship->mounted=0;
	      ship->crystals++;
	      notify(Playernum, Governor, "Dismounted.\n");
	      if(ship->hyper_drive.charge || ship->hyper_drive.ready) {
		  ship->hyper_drive.charge=0;
		  ship->hyper_drive.ready=0;
		  notify(Playernum, Governor, "Discharged.\n");
	      }
	      if(ship->laser && ship->fire_laser) {
		  ship->fire_laser = 0;
		  notify(Playernum, Governor, "Laser deactivated.\n");
	      }
	  } else {
	      notify(Playernum, Governor, "Weird error in 'mount'.\n");
	      free(ship);
	      continue;
	  }
	  putship(ship);
	  free(ship);
      } else
	  free(ship);
}

void use_fuel(shiptype *s, double amt)
{
  s->fuel -= amt;
  s->mass -= amt * MASS_FUEL;
}

void use_destruct(shiptype *s, int amt)
{
  s->destruct -= amt;
  s->mass -= (double)amt * MASS_DESTRUCT;
}

void use_resource(shiptype *s, int amt)
{
  s->resource -= amt;
  s->mass -= (double)amt * MASS_RESOURCE;
}

void use_popn(shiptype *s, int amt, double mass)
{
  s->popn -= amt;
  s->mass -= (double)amt * mass;
}

void rcv_fuel(shiptype *s, double amt)
{
  s->fuel += amt;
  s->mass += amt * MASS_FUEL;
}

void rcv_resource(shiptype *s, int amt)
{
  s->resource += amt;
  s->mass += (double)amt * MASS_RESOURCE;
}

void rcv_destruct(shiptype *s, int amt)
{
  s->destruct += amt;
  s->mass += (double)amt * MASS_DESTRUCT;
}

void rcv_popn(shiptype *s, int amt, double mass)
{
  s->popn += amt;
  s->mass += (double)amt * mass;
}

void rcv_troops(shiptype *s, int amt, double mass)
{
  s->troops += amt;
  s->mass += (double)amt * mass;
}

void do_transporter(racetype *Race, int Governor, shiptype *s)
{
	int Playernum;
	shiptype *s2;

	Playernum = Race->Playernum;
	
	if(!landed(s)) {
	notify(Playernum, Governor, "Origin ship not landed.\n");
	return;
    }
    if(s->storbits != Dir[Playernum-1][Governor].snum ||
       s->pnumorbits != Dir[Playernum-1][Governor].pnum) {
	sprintf(buf,"Change scope to the planet the ship is landed on!\n");
	notify(Playernum, Governor, buf);
	return;
    }
    if(s->damage) {
	notify(Playernum, Governor, "Origin device is damaged.\n");
	return;
    }
    if (!getship(&s2, (int)s->special.transport.target)) {
	sprintf(buf,"The hopper seems to be blocked.\n");
	notify(Playernum, Governor, buf);
	return;
    }
    if (!s2->alive || s2->type!=OTYPE_TRANSDEV || !s2->on) {
	sprintf(buf,"The target device is not receiving.\n");
	notify(Playernum, Governor, buf);
	free(s2); return;
    }
    if(!landed(s2)) {
	notify(Playernum, Governor, "Target ship not landed.\n");
	free(s2); return;
    }
    if(s2->damage) {
	notify(Playernum, Governor, "Target device is damaged.\n");
	free(s2); return;
    }

    sprintf(buf,"Zap\07!\n");     /* ^G */
    notify(Playernum, Governor, buf);
    /* send stuff to other ship (could be transport device) */
    if (s->resource ) {
	rcv_resource(s2, (int)s->resource);
	sprintf(buf,"%d resources transferred.\n", s->resource);
	notify(Playernum, Governor, buf);
	sprintf(bufr, "%d Resources\n",s->resource);
	use_resource(s, (int)s->resource);
    } else
      bufr[0] = '\0';
    if (s->fuel) {
      rcv_fuel(s2, s->fuel);
      sprintf(buf,"%g fuel transferred.\n", s->fuel);
      notify(Playernum, Governor, buf);
      sprintf(buff, "%g Fuel\n",s->fuel);
      use_fuel(s, s->fuel);
    } else
      buff[0] = '\0';

    if (s->destruct) {
	    rcv_destruct(s2, (int)s->destruct);
      sprintf(buf,"%d destruct transferred.\n", s->destruct);
      notify(Playernum, Governor, buf);
      sprintf(bufd, "%d Destruct\n",s->destruct);
      use_destruct(s, (int)s->destruct);
    } else
      bufd[0] = '\0';

    if (s->popn) {
      s2->mass += s->popn * Race->mass;
      s2->popn += s->popn;

      sprintf(buf,"%d population transferred.\n", s->popn);
      notify(Playernum, Governor, buf);
      sprintf(bufc, "%d %s\n", s->popn,
	      Race->Metamorph ? "tons of biomass" : "population");
      s->mass -= s->popn * Race->mass;
      s->popn -= s->popn;
    } else
      bufc[0] = '\0';

    if (s->crystals) {
      s2->crystals += s->crystals;

      sprintf(buf,"%d crystal(s) transferred.\n", s->crystals);
      notify(Playernum, Governor, buf);
      sprintf(bufx, "%d crystal(s)\n",s->crystals);

      s->crystals = 0;
    } else
      bufx[0] = '\0';

    if (s2->owner!=s->owner) {
      sprintf(telegram_buf,"Audio-vibatory-physio-molecular transport device #");
      sprintf(buf,"%s gave your ship %s the following:\n", Ship(s), Ship(s2));
      strcat(telegram_buf, buf);
      strcat(telegram_buf, bufr);
      strcat(telegram_buf, bufd);
      strcat(telegram_buf, buff);
      strcat(telegram_buf, bufc);
      strcat(telegram_buf, bufm);
      strcat(telegram_buf, bufx);
      warn((int)s2->owner, (int)s2->governor, telegram_buf);
  }

    putship(s2);
    free(s2);
    }

int landed_on(shiptype *s, int shipno)
{
    return(s->whatorbits==LEVEL_SHIP && s->destshipno==shipno);
}

void unload_onto_alien_sector(int Playernum, int Governor, planettype *planet,
			      shiptype *ship, sectortype *sect, int what,
			      int people)
{
    double astrength, dstrength;
    int oldowner, oldgov, oldpopn, old2popn, old3popn;
    int casualties, casualties2, casualties3;
    int absorbed, defense;
    racetype *Race, *alien;

    if(people<=0) {
	notify(Playernum, Governor, "You have to unload to assault alien sectors.\n");
	return;
    }
    ground_assaults[Playernum-1][sect->owner-1][Dir[Playernum-1][Governor].snum] += 1;
    Race  = races[Playernum-1];
    alien = races[sect->owner-1];
/* races find out about each other */
    alien->translate[Playernum-1] = MIN(alien->translate[Playernum-1]+5, 100);
    Race->translate[sect->owner-1] = MIN(Race->translate[sect->owner-1]+5,100);

    oldowner = (int)sect->owner;
    oldgov = Stars[Dir[Playernum-1][Governor].snum]->governor[sect->owner-1];

    if(what==CIV)
	ship->popn -= people;
    else
	ship->troops -= people;
    ship->mass -= people*Race->mass;
    sprintf(buf, "%d %s unloaded...\n", people, what==CIV ? "civ" : "mil");
    notify(Playernum, Governor, buf);
    sprintf(buf, "Crew compliment %d civ  %d mil\n", ship->popn, ship->troops);
    notify(Playernum, Governor, buf);

    sprintf(buf, "%d %s assault %d civ/%d mil\n",
	    people, what==CIV ? "civ" : "mil", sect->popn, sect->troops);

    notify(Playernum, Governor, buf);
    oldpopn = people;
    old2popn = sect->popn;
    old3popn = sect->troops;

    defense = Defensedata[sect->condition];
    ground_attack(Race, alien, &people, what, &sect->popn, &sect->troops,
		  (int)ship->armor, defense,
		  1.0-(double)ship->damage/100.0, alien->likes[sect->condition],
		  &astrength, &dstrength,
		  &casualties, &casualties2, &casualties3);
    sprintf(buf,"Attack: %.2f   Defense: %.2f.\n", astrength, dstrength);
    notify(Playernum, Governor, buf);

    if (!(sect->popn + sect->troops)) {     /* we got 'em */
/* mesomorphs absorb the bodies of their victims */
	absorbed = 0;
	if(Race->absorb) {
	    absorbed = int_rand(0, old2popn+old3popn);
	    sprintf(buf, "%d alien bodies absorbed.\n", absorbed);
	    notify(Playernum, Governor, buf);
	    sprintf(buf, "Metamorphs have absorbed %d bodies!!!\n",
		    absorbed);
	    notify(oldowner, oldgov, buf);
	}
	if(what==CIV)
	    sect->popn = people+absorbed;
	else if(what==MIL) {
	    sect->popn = absorbed;
	    sect->troops = people;
	}
	sect->owner = Playernum;
	adjust_morale(Race, alien, (int)alien->fighters);
    } else {		/* retreat */
	absorbed = 0;
	if(alien->absorb) {
	    absorbed = int_rand(0, oldpopn-people);
	    sprintf(buf, "%d alien bodies absorbed.\n", absorbed);
	    notify(oldowner, oldgov, buf);
	    sprintf(buf, "Metamorphs have absorbed %d bodies!!!\n", absorbed);
	    notify(Playernum, Governor, buf);
	    sect->popn += absorbed;
	}
	/* load them back up */
	sprintf(buf, "Loading %d %s\n", people, what==CIV ? "civ" : "mil");
	notify(Playernum, Governor, buf);
	if(what==CIV)
	    ship->popn += people;
	else
	    ship->troops += people;
	ship->mass -= people*Race->mass;
	adjust_morale(alien, Race, (int)Race->fighters);
    }
    sprintf(telegram_buf, "/%s/%s: %s [%d] %s assaults %s [%d] %c(%d,%d) %s\n",
	    Stars[Dir[Playernum-1][Governor].snum]->name,
	    Stars[Dir[Playernum-1][Governor].snum]->pnames[Dir[Playernum-1][Governor].pnum],
	    Race->name, Playernum, Ship(ship), alien->name, alien->Playernum,
	    Dessymbols[sect->condition], ship->land_x, ship->land_y,
	    (sect->owner==Playernum ? "VICTORY" : "DEFEAT"));

    if (sect->owner==Playernum) {
	sprintf(buf,"VICTORY! The sector is yours!\n");
	notify(Playernum, Governor, buf);
	sprintf(buf,"Sector CAPTURED!\n");
	strcat(telegram_buf, buf);
	if (people) {
	    sprintf(buf,"%d %s move in.\n", people, what==CIV ? "civilians" : "troops");
	    notify(Playernum, Governor, buf);
	}
	planet->info[Playernum-1].numsectsowned++;
	planet->info[Playernum-1].mob_points += (int)sect->mobilization;
	planet->info[oldowner-1].numsectsowned--;
	planet->info[oldowner-1].mob_points -= (int)sect->mobilization;
    } else {
	sprintf(buf,"The invasion was repulsed; try again.\n");
	notify(Playernum, Governor, buf);
	sprintf(buf,"You fought them off!\n");
	strcat(telegram_buf, buf);
    }
    if (!(sect->popn + sect->troops + people)) {
	sprintf(buf,"You killed all of them!\n");
	strcat(telegram_buf, buf);
/* increase modifier */
	Race->translate[oldowner-1] = MIN(Race->translate[oldowner-1]+5, 100);
    }
    if (!people) {
	sprintf(buf,"Oh no! They killed your party to the last man!\n");
	notify(Playernum, Governor, buf);
/* increase modifier */
	alien->translate[Playernum-1]=MIN(alien->translate[Playernum-1]+5, 100);
    }
    putrace(alien);
    putrace(Race);
		
    sprintf(buf,"Casualties: You: %d civ/%d mil, Them: %d %s\n",
	    casualties2, casualties3, casualties, what==CIV ? "civ" : "mil");
    strcat(telegram_buf, buf);
    warn(oldowner, oldgov, telegram_buf);
    sprintf(buf,"Casualties: You: %d %s, Them: %d civ/%d mil\n",
	    casualties, what==CIV ? "civ" : "mil", casualties2, casualties3);
    notify(Playernum, Governor, buf);
    return;
}

