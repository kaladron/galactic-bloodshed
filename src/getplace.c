/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *  Getplace -- returns directory level from string and current Dir
 *  Dispplace -- returns string from directory level
 *  testship(ship) -- tests various things for the ship.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "races.h"
#include "ships.h"
#include "power.h"
#include "buffers.h"

char Disps[PLACENAMESIZE];
placetype Getplace(int, int, char *, int);
placetype Getplace2(int, int, char *, placetype *, int, int);
char *Dispshiploc_brief(shiptype *);
char *Dispshiploc(shiptype *);
char *Dispplace(int, int, placetype *);
int testship(int, int, shiptype *);
#include "files_shl.p"
#include "shlmisc.p"
#include "GB_server.p"

placetype Getplace(int Playernum, int Governor, char *string, int ignoreexpl)
{
  placetype where;	/* return value */
  racetype *Race;
  int God;
 
  Bzero(where);

  Race = races[Playernum-1];
  God = Race->God;
 
  where.err = 0;

  switch (*string) {
  case '/':
    where.level=LEVEL_UNIV;	/* scope = root (universe) */
    where.snum=0;
    where.pnum= where.shipno=0;
    return(Getplace2(Playernum, Governor, string+1,&where,ignoreexpl,God));
  case '#':
    sscanf(++string,"%hd",&where.shipno);
    if(!getship(&where.shipptr, where.shipno)){
      DontOwnErr(Playernum,Governor, where.shipno);
      where.err = 1;
      return where;
    }
    if ( (where.shipptr->owner==Playernum || ignoreexpl || God)
	&& (where.shipptr->alive || God)) {
      where.level = LEVEL_SHIP;
      where.snum = where.shipptr->storbits;
      where.pnum = where.shipptr->pnumorbits;
      free(where.shipptr);
      return where;
    } else {
      where.err = 1;
      free(where.shipptr);
      return where;
    }
  case '-':
    /* no destination */
    where.level = LEVEL_UNIV;
    return where;
  default:
    /* copy current scope to scope */
    where.level = Dir[Playernum-1][Governor].level;
    where.snum = Dir[Playernum-1][Governor].snum;
    where.pnum = Dir[Playernum-1][Governor].pnum;
    if (where.level==LEVEL_SHIP)
      where.shipno = Dir[Playernum-1][Governor].shipno;
    if (*string==CHAR_CURR_SCOPE)
      return where;
    else
      return Getplace2(Playernum, Governor, string,&where, ignoreexpl,God);
  }
}

placetype Getplace2(int Playernum, int Governor, char *string,
		    placetype *where, int ignoreexpl, int God)
{
  char substr[NAMESIZE];
  planettype *p;
  register int i,l;
  int tick;

  if (where->err || *string=='\0' || *string=='\n')
    return(*where);		/* base cases */
  else if (*string=='.') {
    if (where->level==LEVEL_UNIV) {
      sprintf(buf,"Can't go higher.\n");
      notify(Playernum, Governor, buf);
      where->err=1;
      return(*where);
    } else {
      if (where->level==LEVEL_SHIP) {
	(void)getship(&where->shipptr,where->shipno);
	where->level = where->shipptr->whatorbits;
	/* Fix 'cs .' for ships within ships. Maarten */
	if(where->level==LEVEL_SHIP)
	  where->shipno = where->shipptr->destshipno;
	free(where->shipptr);
      } else 
	where->level--;
      while (*string=='.') string++;
      while (*string=='/') string++;
      return(Getplace2(Playernum, Governor,string,where,ignoreexpl,God));
    }
  } else {
    /* is a char string, name of something */
    sscanf(string,"%[^/ \n]",substr);
    do {
      /*if (isupper(*string) )
	(*string) = tolower(*string);*/
      string++;
    } while (*string!='/' && *string!='\n' && *string!='\0');
    l=strlen(substr);
    if (where->level==LEVEL_UNIV) {
      for (i=0; i<Sdata.numstars; i++)
	if (!strncmp(substr,Stars[i]->name,l)) {
	  where->level=LEVEL_STAR;
	  where->snum = i;
	  if (ignoreexpl ||
	      isset(Stars[where->snum]->explored, Playernum) || God) {
	    tick = (*string=='/');
	    return(Getplace2(Playernum, Governor, string+tick, where, ignoreexpl, God));
	  }
	  sprintf(buf,"You have not explored %s yet.\n",
		  Stars[where->snum]->name);
	  notify(Playernum, Governor, buf);
	  where->err = 1;
	  return(*where);
	}
      if (i>=Sdata.numstars) {
	sprintf(buf,"No such star %s.\n",substr);
	notify(Playernum, Governor, buf);
	where->err=1;
	return(*where);
      }
    } else if (where->level==LEVEL_STAR) {
      for (i=0; i<Stars[where->snum]->numplanets; i++)
	if (!strncmp(substr,Stars[where->snum]->pnames[i],l)) {
	  where->level=LEVEL_PLAN;
	  where->pnum = i;
	  getplanet(&p, (int)where->snum,i);
	  if (ignoreexpl || p->info[Playernum-1].explored || God) {
	    free(p);
	    tick = (*string=='/');
	    return(Getplace2(Playernum, Governor, string+tick,where,ignoreexpl, God));
	  }
	  sprintf(buf,"You have not explored %s yet.\n",
		  Stars[where->snum]->pnames[i]);
	  notify(Playernum, Governor, buf);
	  where->err = 1;
	  free(p);
	  return(*where);
	}
      if (i >= Stars[where->snum]->numplanets) {
	sprintf(buf,"No such planet %s.\n",substr);
	notify(Playernum, Governor, buf);
	where->err=1;
	return(*where);
      }
    } else {
      sprintf(buf,"Can't descend to %s.\n",substr);
      notify(Playernum, Governor, buf);
      where->err=1;
      return(*where);
    }
  }
}

char *Dispshiploc_brief(shiptype *ship)
{
  int i ;

  switch(ship->whatorbits) {
  case LEVEL_STAR:
    sprintf(Disps, "/%-4.4s", Stars[ship->storbits]->name);
    return(Disps);
  case LEVEL_PLAN:
    sprintf(Disps, "/%s", Stars[ship->storbits]->name) ;
    for (i = 2; (Disps[i] && (i < 5)); i++)  ;
    sprintf(Disps + i, "/%-4.4s", 
	    Stars[ship->storbits]->pnames[ship->pnumorbits]);
    return(Disps);
  case LEVEL_SHIP:
    sprintf(Disps, "#%d", ship->destshipno);
    return(Disps);
  case LEVEL_UNIV:
    sprintf(Disps, "/");
    return(Disps);
  default:
    sprintf(Disps, "error");
    return(Disps);
  }
}

char *Dispshiploc(shiptype *ship)
{
  switch(ship->whatorbits) {
  case LEVEL_STAR:
    sprintf(Disps, "/%s", Stars[ship->storbits]->name);
    return(Disps);
  case LEVEL_PLAN:
    sprintf(Disps, "/%s/%s", Stars[ship->storbits]->name,
	    Stars[ship->storbits]->pnames[ship->pnumorbits]);
    return(Disps);
  case LEVEL_SHIP:
    sprintf(Disps, "#%d", ship->destshipno);
    return(Disps);
  case LEVEL_UNIV:
    sprintf(Disps, "/");
    return(Disps);
  default:
    sprintf(Disps, "error");
    return(Disps);
  }
}

char *Dispplace(int Playernum, int Governor, placetype *where)
{
  switch (where->level) {
  case LEVEL_STAR:
    sprintf(Disps,"/%s",Stars[where->snum]->name);
    return(Disps);
  case LEVEL_PLAN:
    sprintf(Disps,"/%s/%s",Stars[where->snum]->name, 
	    Stars[where->snum]->pnames[where->pnum]);
    return(Disps);
  case LEVEL_SHIP:
    sprintf(Disps,"#%d",where->shipno);
    return(Disps);
  case LEVEL_UNIV:
    return("/");
  default:
    sprintf(buf,"illegal Dispplace val = %d\n",where->level);
    notify(Playernum, Governor, buf);
    where->err = 1;
    return("/");
    break;
  }
}

int testship(int Playernum, int Governor, shiptype *s)
{
  reg int r;

  r=0;
  
  if (!s->alive) {
    sprintf(buf,"%s has been destroyed.\n", Ship(s));
    notify(Playernum, Governor, buf);
    r = 1;
  } else if (s->owner != Playernum || !authorized(Governor, s)) {
    DontOwnErr(Playernum, Governor, (int)s->number);
    r = 1;
  } else {
    if (!s->active) {
      sprintf(buf,"%s is irradiated %d%% and inactive.\n", Ship(s), s->rad);
      notify(Playernum, Governor, buf);
      r = 1;
    } 
/*   if (!s->popn && s->max_crew) {
     sprintf(buf,"%s has no crew and is not a robotic ship.\n", Ship(s));
     notify(Playernum, Governor, buf);
     r = 1;
     } */
  }
  return r;
}

