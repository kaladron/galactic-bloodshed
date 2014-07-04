/* 
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * relation.c -- state relations among players 
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "races.h"
#include "ships.h"
#include "buffers.h"

extern char *Desnames[];

void relation(int, int, int);
char *allied(racetype *, int, int, int);
#include "shlmisc.p"
#include "GB_server.p"

void relation(int Playernum, int Governor, int APcount)
{
int numraces;
int p, q;
racetype *r, *Race;

numraces = Num_races;

if(argn==1) {
    q = Playernum;
} else {
    if(!(q=GetPlayer(args[1]))) {
	notify(Playernum, Governor, "No such player.\n");	    
	return;
    }
}

Race = races[q-1];

sprintf(buf,"\n              Racial Relations Report for %s\n\n",Race->name);
notify(Playernum, Governor, buf);
sprintf(buf," #       know             Race name       Yours        Theirs\n");
notify(Playernum, Governor, buf);
sprintf(buf," -       ----             ---------       -----        ------\n");
notify(Playernum, Governor, buf);
for (p=1; p<=numraces; p++)
    if (p != Race->Playernum) {
	r = races[p-1];
	sprintf(buf,"%2d %s (%3d%%) %20.20s : %10s   %10s\n", p, 
		((Race->God || (Race->translate[p-1] > 30)) && r->Metamorph && (Playernum == q)) ? "Morph" : "     ", Race->translate[p-1], r->name,
		allied(Race, p, 100, (int)Race->God), 
		allied(r, q,(int)Race->translate[p-1], (int)Race->God) );
		notify(Playernum, Governor, buf);
    }
}

char *allied(racetype *r, int p, int q, int God)
{
    if (isset( r->atwar, p)) return "WAR";
    else if (isset( r->allied, p)) return "ALLIED";
    else return "neutral";
}
