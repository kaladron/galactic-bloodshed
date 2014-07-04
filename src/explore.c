/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 * explore.c -- display systems/worlds explored 
 *  (this command written by Dan Corrin, dan@geomech.engrg.uwo.ca)
 *
 * Thu Apr 11 02:19:44 MDT 1991
 *	Made mods to this prior, but forgot the header. :)
 *	Evan Koffler
 */
#include <math.h>
#include <strings.h>
#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
extern char Psymbol[], *Planet_types[];

void colonies_at_star(int, int, racetype *, int, int);
void colonies(int, int, int, int);
void distance(int, int, int);
void star_locations(int, int, int);
void exploration(int, int, int);
void tech_status(int, int, int);
void tech_report_star(int, int, startype *, int, int *, double *, double *);
#include "getplace.p"
#include "files_shl.p"
#include "GB_server.p"
#include "tech.p"
#include "max.p"

void colonies_at_star(int Playernum, int Governor, racetype *Race, int star, int mode)
{
  int i, j ;
  planettype *pl;

  getstar(&(Stars[star]), star);
  if (!isset(Stars[star]->explored,Playernum))
    return ;

  for (i=0; i<Stars[star]->numplanets; i++) {
      getplanet(&pl, star, i);

      if (pl->info[Playernum-1].explored &&
	  pl->info[Playernum-1].numsectsowned &&
	  (!Governor || Stars[star]->governor[Playernum-1]==Governor)) {
	  switch(mode) {
	    case -1:   /* combined report */
	    case 0:    /* colonies */
	      sprintf(buf,
		      " %c %4.4s/%-4.4s%c%4d%3d%5d%8d%3d%6d%5d%6d %3d/%-3d%3.0f/%-3d%3d/%-3d",
		      Psymbol[pl->type],
		      Stars[star]->name,
		      Stars[star]->pnames[i],
		      (pl->info[Playernum-1].autorep ? '*' : ' '),
		      Stars[star]->governor[Playernum-1],
		      pl->info[Playernum-1].numsectsowned,
		      pl->info[Playernum-1].tech_invest,
		      pl->info[Playernum-1].popn,
		      pl->info[Playernum-1].crystals,
		      pl->info[Playernum-1].resource,
		      pl->info[Playernum-1].destruct,
		      pl->info[Playernum-1].fuel,
		      pl->info[Playernum-1].tax,
		      pl->info[Playernum-1].newtax,
		      compatibility(pl, Race),
		      pl->conditions[TOXIC],
		      pl->info[Playernum-1].comread,
		      pl->info[Playernum-1].mob_set  );
	      notify(Playernum, Governor, buf);
	      for (j=1; j<=Num_races; j++)
		  if ((j != Playernum) && (pl->info[j-1].numsectsowned > 0))
		  {
		      sprintf(buf," %d",j);
		      notify(Playernum, Governor, buf);
		  }
	      notify(Playernum, Governor, "\n");
	      if (mode == 0)
		  break ;
	      /* Fall through if (mode == -1) */
	    case 1:   /* production */
	      sprintf(buf,
		      " %c %4.4s/%-4.4s%c%3d%8.4f%8d%3d%6d%5d%6d %6d   %3d%8.2f\n",
		      Psymbol[pl->type],
		      Stars[star]->name, Stars[star]->pnames[i],
		      (pl->info[Playernum-1].autorep ? '*' : ' '),
		      Stars[star]->governor[Playernum-1],
		      pl->info[Playernum-1].prod_tech,
		      pl->total_resources,
		      pl->info[Playernum-1].prod_crystals,
		      pl->info[Playernum-1].prod_res,
		      pl->info[Playernum-1].prod_dest,
		      pl->info[Playernum-1].prod_fuel,
		      pl->info[Playernum-1].prod_money,
		      pl->info[Playernum-1].tox_thresh,
		      pl->info[Playernum-1].est_production) ;
	      notify(Playernum, Governor, buf);
	      break ;
	    default:
	      break ;
	  }
      }
      free(pl);
  }
}

void colonies(int Playernum, int Governor, int APcount, int mode)
{
  int i, star ;
  racetype *Race;
  placetype where;

  switch (mode) {
    case -1:
      notify(Playernum, Governor, "          ========= Colonies Prod Report ==========\n");
      notify(Playernum, Governor, "  Planet     gov sec tech    popn  x   res  des  fuel  tax  cmpt/tox mob   Aliens\n") ;
      notify(Playernum, Governor, "               tox  deposit\n") ;
      break ;
    case 0:
      notify(Playernum, Governor, "          ========== Colonization Report ==========\n");
      notify(Playernum, Governor, "  Planet     gov sec tech    popn  x   res  des  fuel  tax  cmpt/tox mob  Aliens\n") ;
      break ;
    case 1:
      notify(Playernum, Governor, "          ============ Production Report ==========\n");
      notify(Playernum, Governor, "  Planet     gov    tech deposit  x   res  des  fuel    tax   tox  est prod\n") ;
      break ;
    default:
      notify(Playernum, Governor, "          =============== Unknown Report ==========\n");
      return ;
  }

  Race = races[Playernum-1];
  getsdata(&Sdata);

  if (argn < 2)
      for (star = 0; star < Sdata.numstars; star++)
	  colonies_at_star(Playernum, Governor, Race, star, mode) ;
  else
      for (i=1; i<argn; i++) {
	  where = Getplace(Playernum, Governor, args[i], 0) ;
	  if (where.err || (where.level==LEVEL_UNIV) || (where.level==LEVEL_SHIP))
	  {
	      sprintf(buf, "Bad location `%s'.\n", args[i]) ;
	      notify(Playernum, Governor, buf) ;
	      continue ;
	  }
	  else    /* ok, a proper location */
	      colonies_at_star(Playernum, Governor, Race, where.snum, mode) ;
      }
  notify(Playernum, Governor, "\n") ;
}


void distance(int Playernum, int Governor, int APcount)
{
  placetype from, to;
  planettype *p;
  double x0, y0, x1, y1, dist;
  shiptype *ship;

  if(argn<3) {
      notify(Playernum, Governor, "Syntax: 'distance <from> <to>'.\n");
      return;
  }

  from = Getplace(Playernum, Governor, args[1], 1);
  if(from.err) {
      sprintf(buf, "Bad scope '%s'.\n", args[1]);
      notify(Playernum, Governor, buf);
      return;
  }
  to = Getplace(Playernum, Governor, args[2], 1);
  if(to.err) {
      sprintf(buf, "Bad scope '%s'.\n", args[2]);
      notify(Playernum, Governor, buf);
  }

  x0 = 0.0; 
  y0 = 0.0; 
  x1=0.0; 
  y1=0.0;
  /* get position in absolute units */
  if(from.level==LEVEL_SHIP) {
      (void)getship(&ship, from.shipno);
      if(ship->owner != Playernum) {
	  notify(Playernum, Governor, "Nice try.\n");
	  free(ship);
	  return;
      }
      x0=ship->xpos;
      y0=ship->ypos;
      free(ship);
  } else if(from.level==LEVEL_PLAN) {
      getplanet(&p, (int)from.snum,(int)from.pnum);
      x0=p->xpos+Stars[from.snum]->xpos;
      y0=p->ypos+Stars[from.snum]->ypos;
      free(p);
  } else if(from.level==LEVEL_STAR) {
      x0=Stars[from.snum]->xpos;
      y0=Stars[from.snum]->ypos;
  }

  if(to.level==LEVEL_SHIP) {
      (void)getship(&ship, to.shipno);
      if(ship->owner != Playernum) {
	  notify(Playernum, Governor, "Nice try.\n");
	  free(ship);
	  return;
      }
      x1=ship->xpos;
      y1=ship->ypos;
      free(ship);
  } else if(to.level==LEVEL_PLAN) {
      getplanet(&p, (int)to.snum,(int)to.pnum);
      x1=p->xpos+Stars[to.snum]->xpos;
      y1=p->ypos+Stars[to.snum]->ypos;
      free(p);
  } else if(to.level==LEVEL_STAR) {
      x1=Stars[to.snum]->xpos;
      y1=Stars[to.snum]->ypos;
  }
  /* compute the distance */
  dist = sqrt( Distsq(x0,y0,x1,y1));
  sprintf(buf, "Distance = %f\n", dist);
  notify(Playernum, Governor, buf);
}

void star_locations(int Playernum, int Governor, int APcount)
{
  reg int i;
  double dist, x, y;
  int max;

  x = Dir[Playernum-1][Governor].lastx[1];
  y = Dir[Playernum-1][Governor].lasty[1];
  
  if(argn > 1)
      max=atoi(args[1]);
  else
      max = 999999;

  for(i=0; i<Sdata.numstars; i++) {
      dist=sqrt(Distsq(Stars[i]->xpos, Stars[i]->ypos, x, y));
      if((int)dist <= max) {
	  sprintf(buf, "(%2d) %20.20s (%8.0f,%8.0f) %7.0f\n", i,
		  Stars[i]->name, Stars[i]->xpos, Stars[i]->ypos, dist);
	  notify(Playernum, Governor, buf);
      }
  }
}

void exploration(int Playernum, int Governor, int APcount)
{
  int star,starq,i,j;
  planettype *pl;
  placetype where;
  int inhab = 0;
  racetype *Race;

  starq = -1;

  if(argn==2) {
      where = Getplace(Playernum, Governor, args[1], 0);
      if(where.err) {
	  sprintf(buf, "explore: bad scope.\n");
	  notify(Playernum, Governor, buf);
	  return;
      } else if(where.level==LEVEL_SHIP || where.level==LEVEL_UNIV) {
	  sprintf(buf, "Bad scope '%s'.\n", args[1]);
	  notify(Playernum, Governor, buf);
	  return;
      }
      starq = where.snum;
  }

  Race = races[Playernum-1];

  getsdata(&Sdata);
  sprintf(buf,"         ========== Exploration Report ==========\n");
  notify(Playernum, Governor, buf);
  sprintf(buf," Global action points : [%2d]\n",Sdata.AP[Playernum-1]);
  notify(Playernum, Governor, buf);
  sprintf(buf," Star  (stability)[AP]   #  Planet [Attributes] Type (Compatibility)\n");
  notify(Playernum, Governor, buf);
  for (star=0; star<Sdata.numstars; star++)
      if((starq == -1) || (starq == star)) {
	  getstar(&(Stars[star]), star);
	  if (isset(Stars[star]->explored,Playernum))
	      for (i=0; i<Stars[star]->numplanets; i++) {
		  getplanet(&pl,star,i);
		  if (i == 0)
		      if (Race->tech >= TECH_SEE_STABILITY) {
			  sprintf(buf,"\n%13s (%2d)[%2d]\n",Stars[star]->name,Stars[star]->stability,
				  Stars[star]->AP[Playernum-1]);
			  notify(Playernum, Governor, buf);
		      } else {
			  sprintf(buf,"\n%13s (/?/?)[%2d]\n",Stars[star]->name,
				  Stars[star]->AP[Playernum-1]);
			  notify(Playernum, Governor, buf);
		      }

		  sprintf(buf,"\t\t      ");
		  notify(Playernum, Governor, buf);

		  sprintf(buf,"  #%d. %-15s [ ",i+1,Stars[star]->pnames[i]);
		  notify(Playernum, Governor, buf);
		  if (pl->info[Playernum-1].explored)  {
		      sprintf(buf,"Ex ");
		      notify(Playernum, Governor, buf);
		      if (pl->info[Playernum-1].autorep) {
			  sprintf(buf,"Rep ");
			  notify(Playernum, Governor, buf);
		      }
		      if (pl->info[Playernum-1].numsectsowned) {
			  sprintf(buf,"Inhab ");
			  notify(Playernum, Governor, buf);
		      }
		      if (pl->slaved_to) {
			  sprintf(buf,"SLAVED ");
			  notify(Playernum, Governor, buf);
		      }
		      inhab = 0;
		      for (j=1; j<=Num_races; j++)
			  if (j!=Playernum && pl->info[j-1].numsectsowned) {
			      sprintf(buf,"%d ",j);
			      notify(Playernum, Governor, buf);
			  }
		      if (pl->conditions[TOXIC] > 70) {
			  sprintf(buf,"TOXIC ");
			  notify(Playernum, Governor, buf);
		      }
		      sprintf(buf, "] %s %2.0f%%\n", Planet_types[pl->type],
			      compatibility(pl,Race));
		      notify(Playernum, Governor, buf);
		  } else {
		      sprintf(buf,"No Data ]\n");
		      notify(Playernum, Governor, buf);
		  }
		  free(pl);
	      }
      }
}

void tech_status(int Playernum, int Governor, int APcount)
{
  int star, k;
  placetype where;
  double total_gain = 0.0;
  double total_max_gain = 0.0;
  int total_invest = 0;

  getsdata(&Sdata);

  sprintf(buf,"             ========== Technology Report ==========\n\n");
  notify(Playernum, Governor, buf);

  sprintf(buf, "       Planet          popn    invest    gain   ^gain\n");
  notify(Playernum, Governor, buf);

  if (argn == 1) {
      for (star = 0; star < Sdata.numstars; star++) {
	  getstar(&(Stars[star]), star);
	  tech_report_star(Playernum, Governor, Stars[star], star,
			   &total_invest, &total_gain, &total_max_gain);
      }
  } else { /* Several arguments */
      for (k = 1; k < argn; k++) {
	  where = Getplace(Playernum, Governor, args[k], 0);
	  if (where.err || where.level==LEVEL_UNIV || where.level==LEVEL_SHIP) {
	      sprintf(buf, "Bad location `%s'.\n", args[k]);
	      notify(Playernum, Governor, buf);
	      continue;
	  } else {    /* ok, a proper location */
	      star = where.snum;
	      getstar(&Stars[star], star);
	      tech_report_star(Playernum, Governor, Stars[star], star,
			       &total_invest, &total_gain, &total_max_gain);
	  }
      }
  }
  sprintf(buf, "       Total Popn:  %7ld\n", Power[Playernum-1].popn);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Tech: %31d%8.3lf%8.3lf\n",
	  total_invest, total_gain, total_max_gain);
  notify(Playernum, Governor, buf);
}


void tech_report_star(int Playernum, int Governor, startype *star, int snum,
		      int *t_invest, double *t_gain, double *t_max_gain)
{
  int i;
  planettype *pl;
  char str[200];
  double gain, max_gain;

  if (isset(star->explored,Playernum) &&
      (!Governor || star->governor[Playernum-1]==Governor)) {
      for (i = 0; i < star->numplanets; i++) {
	  getplanet(&pl,snum,i);
	  if (pl->info[Playernum-1].explored 
	      && pl->info[Playernum-1].numsectsowned)  {
	      sprintf(str,"%s/%s%s", star->name, star->pnames[i],
		      (pl->info[Playernum-1].autorep ? "*" : ""));
	      sprintf(buf, "%16.16s %10d%10d%8.3lf%8.3lf\n", str,
		      pl->info[Playernum-1].popn,
		      pl->info[Playernum-1].tech_invest,
		      gain = tech_prod((int)pl->info[Playernum-1].tech_invest,
				       (int)pl->info[Playernum-1].popn),
		      max_gain = tech_prod((int)pl->info[Playernum-1].prod_res,
					   (int)pl->info[Playernum-1].popn));
	      notify(Playernum, Governor, buf);
	      *t_invest += pl->info[Playernum-1].tech_invest;
	      *t_gain += gain;
	      *t_max_gain += max_gain;
	  }
	  free(pl);
      }
  }
}


