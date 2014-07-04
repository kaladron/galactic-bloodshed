/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  ship -- report -- stock -- tactical -- stuff on ship
 *
 *  Command "factories" programmed by varneyml@gb.erc.clarkson.edu
 */

#define REPORT	0
#define STOCK	1
#define TACTICAL 2
#define SHIP	3
#define STATUS	4
#define WEAPONS	5
#define FACTORIES 6

#define PLANET 1

#include <ctype.h>
#include <math.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

extern char Shipltrs[];

char Caliber[]={' ', 'L', 'M', 'H'};
char shiplist[256];

static unsigned char Status,SHip,Stock,Report,Tactical,Weapons,Factories,first;

struct reportdata {
  unsigned char type;	/* ship or planet */
  shiptype *s;
  planettype *p;
  short n;
  unsigned char star;
  unsigned char pnum;
  double x;
  double y;
};

racetype *Race;
struct reportdata *rd;
int enemies_only, who;

void rst(int, int, int, int);
void ship_report(int, int, int, unsigned char []);
void plan_getrships(int, int, int, int);
void star_getrships(int, int, int);
int Getrship(int, int, int);
void Free_rlist(void);
int listed(int, char *);
#include "files_shl.p"
#include "GB_server.p"
#include "shlmisc.p"
#include "getplace.p"
#include "shootblast.p"
#include "fire.p"
#include "build.p"
#include "max.p"
#include "order.p"

void rst(int Playernum, int Governor, int APcount, int Rst)
{
  int shipno;
  reg int shn,i;
  int n_ships, num;
  unsigned char Report_types[NUMSTYPES];
  
  for (i=0; i<NUMSTYPES; i++) Report_types[i]=1;
  enemies_only = 0;
  Num_ships = 0;
  first = 1;
  switch (Rst) {
    case REPORT:	Report = 1;
      Weapons = Status = Stock = SHip = Tactical = Factories = 0;
      break;
    case STOCK:	Stock = 1;
      Weapons = Status = Report = SHip = Tactical = Factories = 0;
      break;
    case TACTICAL:	Tactical = 1;
      Weapons = Status = Report = SHip = Stock = Factories = 0;
      break;
    case SHIP:	SHip = Report = Stock = 1;
      Tactical = 0;
      Weapons = Status = Factories = 1;
      break;
    case STATUS:	Status = 1;
      Weapons = Report = Stock = Tactical = SHip = Factories = 0;
      break;
    case WEAPONS: Weapons = 1;
      Status = Report = Stock = Tactical = SHip = Factories = 0;
      break;
    case FACTORIES:	Factories = 1;
      Status = Report = Stock = Tactical = SHip = Weapons = 0;
      break;
  }
  n_ships = Numships();
  rd = (struct reportdata *)malloc(sizeof(struct reportdata) * 
				   (n_ships + Sdata.numstars * MAXPLANETS));
  /* (one list entry for each ship, planet in universe) */
  
  Race = races[Playernum-1];

  if (argn==3) {
      if(isdigit(args[2][0]))
	  who = atoi(args[2]);
      else {
	  who = 999;	/* treat args[2] as a list of ship types */
	  strcpy(shiplist, args[2]);
      }
  } else
      who = 0;
  
  if (argn>=2) {
      if (*args[1] == '#' || isdigit(*args[1])) {
	  /* report on a couple ships */
	  int l=1;
	  while (l<MAXARGS && *args[l]!='\0') {
	      sscanf(args[l] + (*args[l]=='#'),"%d",&shipno);
	      if ( shipno > n_ships || shipno<1) {
		  sprintf (buf,"rst: no such ship #%d \n",shipno);
		  notify(Playernum, Governor, buf);
		  free(rd);
		  return ;
	      }
	      (void)Getrship(Playernum, Governor, shipno);
	      num = Num_ships;
	      if(rd[Num_ships-1].s->whatorbits != LEVEL_UNIV) {
		  star_getrships(Playernum, Governor, (int)rd[num-1].s->storbits);
		  ship_report(Playernum,Governor,num-1,Report_types);
	      } else 
		  ship_report(Playernum,Governor,num-1,Report_types);
	      l++;
	  }
	  Free_rlist();
	  return;
      } else {
	  int l;
	  l = strlen(args[1]);
	  for (i=0; i<NUMSTYPES; i++) Report_types[i]=0;
	
	  while (l--) {
	      i = NUMSTYPES;
	      while (--i && Shipltrs[i]!=args[1][l]);
	      if (Shipltrs[i]!=args[1][l]) {
		  sprintf(buf,"'%c' -- no such ship letter\n",args[1][l]);
		  notify(Playernum, Governor, buf);
	      } else
		  Report_types[i] = 1;
          }
      }
  }
  
  switch (Dir[Playernum-1][Governor].level) {
    case LEVEL_UNIV:
      if(!(Rst==TACTICAL && argn<2)) {
	  shn = Sdata.ships;
	  while (shn && Getrship(Playernum, Governor, shn))
	      shn = rd[Num_ships-1].s->nextship;
      
	  for (i=0; i<Sdata.numstars; i++)
	      star_getrships(Playernum, Governor, i);
	  for (i=0; i<Num_ships; i++)
	      ship_report(Playernum, Governor, i, Report_types);
      } else {
	  notify(Playernum, Governor, "You can't do tactical option from universe level.\n");
	  free(rd);	/* nothing allocated */
	  return;
      }
      break;
    case LEVEL_PLAN:
      plan_getrships(Playernum, Governor, Dir[Playernum-1][Governor].snum, Dir[Playernum-1][Governor].pnum);
      for (i=0; i<Num_ships; i++)
	  ship_report(Playernum, Governor, i, Report_types);
      break; 
    case LEVEL_STAR:
      star_getrships(Playernum, Governor, Dir[Playernum-1][Governor].snum);
      for (i=0; i<Num_ships; i++)
	  ship_report(Playernum, Governor, i, Report_types);
      break; 
    case LEVEL_SHIP:
      (void)Getrship(Playernum, Governor, Dir[Playernum-1][Governor].shipno);
      ship_report(Playernum, Governor, 0, Report_types);       /* first ship report */
      shn = rd[0].s->ships;
      Num_ships = 0;	
    
      while (shn && Getrship(Playernum, Governor, shn))
	  shn = rd[Num_ships-1].s->nextship;
    
      for (i=0; i<Num_ships; i++)
	  ship_report(Playernum, Governor, i, Report_types);
      break;
  }
  Free_rlist();
}

void ship_report(int Playernum, int Governor, int indx, unsigned char rep_on[])
{
  shiptype *s;
  planettype *p;
  int shipno;
  reg int i,sight, caliber;
  placetype where;
  char orb[PLACENAMESIZE];
  char strng[COMMANDSIZE],locstrn[COMMANDSIZE];
  char tmpbuf1[10], tmpbuf2[10], tmpbuf3[10], tmpbuf4[10];
  double Dist;
  
  /* last ship gotten from disk */
  s = rd[indx].s;
  p = rd[indx].p;
  shipno = rd[indx].n;
  
  /* launched canister, non-owned ships don't show up */
  if ( (rd[indx].type==PLANET && p->info[Playernum-1].numsectsowned)
      || (rd[indx].type!=PLANET && s->alive && s->owner==Playernum &&
	  authorized(Governor, s) &&
	  rep_on[s->type] && !(s->type==OTYPE_CANIST && !s->docked) &&
	  !(s->type==OTYPE_GREEN  && !s->docked) )) 
    {
    if (rd[indx].type!=PLANET && Stock) 
      {
      if (first) {
	sprintf(buf,"    #       name        x  hanger   res        des         fuel      crew/mil\n");
	notify(Playernum, Governor, buf);
	if (!SHip)
	  first=0;
	}
      sprintf(buf,"%5d %c %14.14s%3u%4u:%-3u%5u:%-5d%5u:%-5d%7.1f:%-6d%u/%u:%d\n",
	      shipno, Shipltrs[s->type], 
	      (s->active ? s->name : "INACTIVE"),
	      s->crystals, s->hanger, s->max_hanger,
	      s->resource, Max_resource(s), s->destruct, Max_destruct(s),
	      s->fuel, Max_fuel(s), s->popn, s->troops, s->max_crew);
      notify(Playernum, Governor, buf);
      }
    
    if (rd[indx].type!=PLANET && Status) {
      if (first) {
	sprintf(buf,"    #       name       las cew hyp    guns   arm tech spd cost  mass size\n");
	notify(Playernum, Governor, buf);
	if (!SHip)
	  first=0;
	}
      sprintf(buf,"%5d %c %14.14s %s%s%s%3u%c/%3u%c%4u%5.0f%4u%5u%7.1f%4u",
	      shipno, Shipltrs[s->type],
	      (s->active ? s->name : "INACTIVE"),
	      s->laser ? "yes " : "    ",
	      s->cew ? "yes " : "    ", 
	      s->hyper_drive.has ? "yes " : "    ",
	      s->primary, Caliber[s->primtype], s->secondary, Caliber[s->sectype],
	      Armor(s), s->tech,Max_speed(s),Cost(s),Mass(s),Size(s) ) ;
      notify(Playernum, Governor, buf);
      if(s->type==STYPE_POD) {
	  sprintf(buf, " (%d)", s->special.pod.temperature);
	  notify(Playernum, Governor, buf);
      }
      notify(Playernum, Governor, "\n");
  }
    
    if (rd[indx].type!=PLANET && Weapons) {
      if (first) {
	sprintf(buf,"    #       name      laser   cew     safe     guns    damage   class\n");
	notify(Playernum, Governor, buf);
	if (!SHip)
	  first=0;
	}
      sprintf(buf,"%5d %c %14.14s %s  %3d/%-4d  %4d  %3d%c/%3d%c    %3d%%  %c %s\n",
	      shipno, Shipltrs[s->type],
	      (s->active ? s->name : "INACTIVE"),
	      s->laser ? "yes ": "    ", s->cew , s->cew_range,
	      (int)((1.0-.01*s->damage)*s->tech/4.0),
	      s->primary, Caliber[s->primtype], 
	      s->secondary, Caliber[s->sectype],
	      s->damage, s->type==OTYPE_FACTORY ? Shipltrs[s->build_type] : ' ',
	      ((s->type == OTYPE_TERRA) || 
	       (s->type == OTYPE_PLOW)) ? "Standard" :  s->class) ;
      notify(Playernum, Governor, buf);
      }

    if (rd[indx].type!=PLANET && Factories && (s->type == OTYPE_FACTORY)) {
      if (first) {
        sprintf(buf,"   #    Cost Tech Mass Sz A Crw Ful Crg Hng Dst Sp Weapons Lsr CEWs Range Dmg\n");
        notify(Playernum, Governor, buf);
        if (!SHip)
          first=0;
        }
      if ((s->build_type==0)||(s->build_type==OTYPE_FACTORY)) {
	sprintf(buf,"%5d               (No ship type specified yet)                      75% (OFF)", shipno);
	notify(Playernum, Governor, buf); } else {
      if (s->primtype) sprintf(tmpbuf1,"%2d%s",s->primary,s->primtype==LIGHT?
	"L":s->primtype==MEDIUM?"M":s->primtype==HEAVY?"H":"N");
      else strcpy(tmpbuf1,"---");
      if (s->sectype) sprintf(tmpbuf2,"%2d%s",s->secondary,s->sectype==LIGHT?
	"L":s->sectype==MEDIUM?"M":s->sectype==HEAVY?"H":"N");
      else strcpy(tmpbuf2,"---");
      if (s->cew) sprintf(tmpbuf3,"%4d",s->cew);
      else strcpy(tmpbuf3,"----");
      if (s->cew) sprintf(tmpbuf4,"%5d",s->cew_range);
      else strcpy(tmpbuf4,"-----");
      sprintf(buf,"%5d %c%4d%6.1f%5.1f%3d%2d%4d%4d%4d%4d%4d %s%1d %s/%s %s %s %s %02d%%%s\n",
		shipno, Shipltrs[s->build_type],
		s->build_cost, s->complexity, s->base_mass, ship_size(s),
		s->armor, s->max_crew, s->max_fuel, s->max_resource,
		s->max_hanger, s->max_destruct,
		s->hyper_drive.has?(s->mount?"+":"*"):" ",
		s->max_speed, tmpbuf1, tmpbuf2, s->laser?"yes":" no",
		tmpbuf3, tmpbuf4, s->damage,
		s->damage?(s->on?"":"*"):"");
      notify(Playernum, Governor, buf); }
      }
    
    if (rd[indx].type!=PLANET && Report) {
      if (first) {
	sprintf(buf, " #      name       gov dam crew mil  des fuel sp orbits     destination\n");
	notify(Playernum, Governor, buf);
	if (!SHip)
	  first=0;
	}
      if (s->docked)
	if (s->whatdest == LEVEL_SHIP) 
	  sprintf(locstrn,"D#%d", s->destshipno) ;
	else
	  sprintf(locstrn,"L%2d,%-2d",s->land_x,s->land_y);
      else 
	if (s->navigate.on)
	  sprintf(locstrn,"nav:%d (%d)", s->navigate.bearing, 
		  s->navigate.turns);
	else 
	  strcpy(locstrn, prin_ship_dest(Playernum, Governor, s)) ;

      if (!s->active) {
	sprintf(strng, "INACTIVE(%d)", s->rad);
	notify(Playernum, Governor, buf);
	}

      sprintf(buf,"%c%-5d %12.12s %2d %3u%5u%4u%5u%5.0f %c%1u %-10s %-18s\n",
	      Shipltrs[s->type], shipno,
	      (s->active ? s->name : strng), s->governor,
	      s->damage, s->popn, s->troops,
	      s->destruct, s->fuel,
	      s->hyper_drive.has ? (s->mounted ? '+' : '*') : ' ', s->speed, 
	      Dispshiploc_brief(s), locstrn,
	      0);
      notify(Playernum, Governor, buf);
      }
    
    if (Tactical) {
      int fev=0,fspeed=0,defense,fdam=0;
      double tech;

      sprintf(buf,"\n  #         name        tech    guns  armor size dest   fuel dam spd evad               orbits\n");
      notify(Playernum, Governor, buf);
      
      if (rd[indx].type==PLANET) {
	tech = Race->tech;
	/* tac report from planet */
	sprintf(buf,"(planet)%15.15s%4.0f %4dM           %5u %6u\n",
		Stars[rd[indx].star]->pnames[rd[indx].pnum],
		tech, p->info[Playernum-1].guns,
		p->info[Playernum-1].destruct,
		p->info[Playernum-1].fuel);
	notify(Playernum, Governor, buf);
	caliber = MEDIUM;
	} else {
	  where.level = s->whatorbits;
	  where.snum = s->storbits;
	  where.pnum = s->pnumorbits;
	  tech = s->tech;
	  caliber = current_caliber(s);
	  if((s->whatdest != LEVEL_UNIV || s->navigate.on) &&
	     !s->docked && s->active) {
	    fspeed = s->speed;
	    fev = s->protect.evade;
	    }
	  fdam = s->damage;
	  sprintf(orb, "%30.30s", Dispplace(Playernum, Governor, &where));
	  sprintf(buf,"%3d %c %16.16s %4.0f%3d%c/%3d%c%6d%5d%5u%7.1f%3d%%  %d  %3s%21.22s", 
		  shipno, Shipltrs[s->type], 
		  (s->active ? s->name : "INACTIVE"),
		  s->tech,
		  s->primary, Caliber[s->primtype], 
		  s->secondary, Caliber[s->sectype],
		  s->armor, s->size,
		  s->destruct, s->fuel, s->damage, fspeed,
		  (fev ? "yes" : "   "),
		  orb) ;
	  notify(Playernum, Governor, buf);
	  if (landed(s)) {
	    sprintf(buf," (%d,%d)",s->land_x,s->land_y);
	    notify(Playernum, Governor, buf);
	    }
	  if (!s->active) {
	    sprintf(buf," INACTIVE(%d)",s->rad);
	    notify(Playernum, Governor, buf);
	    }
	  sprintf(buf,"\n");
	  notify(Playernum, Governor, buf);
	  }
      
      sight = 0;
      if(rd[indx].type==PLANET) sight = 1;
      else if(Sight(s)) sight = 1;
      
      /* tactical display */
      sprintf(buf,"\n  Tactical: #  own typ        name   rng   (50%%) size spd evade hit  dam  loc\n");
      notify(Playernum, Governor, buf);
      
      if(sight)
	for (i=0; i<Num_ships; i++) {
	  if (i!=indx &&
	      (Dist = sqrt(Distsq(rd[indx].x, rd[indx].y, 
				  rd[i].x, rd[i].y))) < gun_range(Race, rd[indx].s, (rd[indx].type==PLANET)))
	    if (rd[i].type==PLANET) {
	      /* tac report at planet */
	      sprintf(buf," %13s(planet)          %8.0f\n", 
		      Stars[rd[i].star]->pnames[rd[i].pnum], Dist);
	      notify(Playernum, Governor, buf);
	      } else if(!who || who==rd[i].s->owner ||
			(who==999 && listed((int)rd[i].s->type, shiplist))) {
		/* tac report at ship */
		if ((rd[i].s->owner!=Playernum || !authorized(Governor, rd[i].s)) &&
		    rd[i].s->alive &&
		    rd[i].s->type != OTYPE_CANIST &&
		    rd[i].s->type != OTYPE_GREEN) {
		  int tev=0, tspeed=0, body=0,prob=0;
		  int factor=0;		
		  if((rd[i].s->whatdest != LEVEL_UNIV || rd[i].s->navigate.on) 
		     && !rd[i].s->docked && rd[i].s->active) {
		    tspeed = rd[i].s->speed;
		    tev = rd[i].s->protect.evade;
		    }
		  body = Size(rd[i].s);
		  defense = getdefense(rd[i].s);
		  prob = hit_odds(Dist,&factor,tech,fdam,fev,tev,fspeed,tspeed,body, caliber, defense);
		  if(rd[indx].type!=PLANET &&
		     laser_on(rd[indx].s) && rd[indx].s->focus)
		    prob = prob*prob/100;
		  sprintf(buf,"%13d %s%2d,%1d %c%14.14s %4.0f  %4d   %4d %d  %3s  %3d%% %3u%%%s",
			  rd[i].n, (isset(races[Playernum-1]->atwar,
				rd[i].s->owner))? "-" :
				(isset(races[Playernum-1]->allied,
				rd[i].s->owner)) ? "+" : " ",
			  rd[i].s->owner, rd[i].s->governor,
			  Shipltrs[rd[i].s->type], 
			  rd[i].s->name, Dist,factor,body,tspeed,(tev ? "yes" : "   "), prob,rd[i].s->damage,
			  (rd[i].s->active ? "" : " INACTIVE"));
		  if ((enemies_only==0)||((enemies_only==1)&&(!isset(races[Playernum-1]->allied,rd[i].s->owner)))) { notify(Playernum, Governor, buf);
		  if (landed(rd[i].s)) {
		    sprintf(buf," (%d,%d)",rd[i].s->land_x,rd[i].s->land_y);
		    notify(Playernum, Governor, buf);
		    } else {
		      sprintf(buf,"     ");
		      notify(Playernum, Governor, buf);
		      }
		  sprintf(buf, "\n");
		  notify(Playernum, Governor, buf); }
		  }
		}
	  }
      }
    }
  }


void plan_getrships(int Playernum, int Governor, int snum, int pnum)
{
  reg int shn;
  planettype *p;
  
  getplanet(&(rd[Num_ships].p),snum,pnum);
  p = rd[Num_ships].p;
  /* add this planet into the ship list */
  rd[Num_ships].star = snum;
  rd[Num_ships].pnum = pnum;
  rd[Num_ships].type = PLANET;
  rd[Num_ships].n = 0;
  rd[Num_ships].x = Stars[snum]->xpos + p->xpos;
  rd[Num_ships].y = Stars[snum]->ypos + p->ypos;
  Num_ships++;
  
  if (p->info[Playernum-1].explored) {
    shn = p->ships;
    while (shn && Getrship(Playernum, Governor, shn))
      shn = rd[Num_ships-1].s->nextship;
    } 
  }

void star_getrships(int Playernum, int Governor, int snum)
{
  reg int shn;
  int i;
  
  if (isset(Stars[snum]->explored, Playernum)) {
      shn = Stars[snum]->ships;
      while (shn && Getrship(Playernum, Governor, shn))
	  shn = rd[Num_ships-1].s->nextship;
      for (i=0; i<Stars[snum]->numplanets; i++)
	  plan_getrships(Playernum, Governor, snum, i);
  }
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
int Getrship(int Playernum, int Governor, int shipno)
{
  if (getship(&(rd[Num_ships].s),shipno)) {
      rd[Num_ships].type = 0;
      rd[Num_ships].n = shipno;
      rd[Num_ships].x = rd[Num_ships].s->xpos;
      rd[Num_ships].y = rd[Num_ships].s->ypos;
      Num_ships++;
      return 1;
  } else {
      sprintf(buf,"Getrship: error on ship get (%d).\n",shipno);
      notify(Playernum, Governor, buf);
      return 0;
  }
}

void Free_rlist(void)
{
    reg int i;
    for (i=0; i<Num_ships; i++)
	if (rd[i].type==PLANET)
	    free(rd[i].p);
	else
	    free(rd[i].s);
    free(rd);
}

int listed(int type, char *string)
{
    char *p;

    for(p=string; *p; p++) {
	if(Shipltrs[type]==*p)
	    return 1;
    }
    return 0;
}
