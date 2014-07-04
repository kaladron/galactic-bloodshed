/* Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * tech.c -- increase investment in technological development.
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "power.h"
#include "races.h"
#include "buffers.h"
#include <math.h>
#include <signal.h>
#include <ctype.h>

void technology(int, int, int);
double tech_prod(int, int);
#include "GB_server.p"
#include "shlmisc.p"
#include "files_shl.p"
#include "mobiliz.p"

void technology(int Playernum, int Governor, int APcount)
{
short invest;
planettype *p;

if (Dir[Playernum-1][Governor].level != LEVEL_PLAN) {
    sprintf(buf,"scope must be a planet (%d).\n", Dir[Playernum-1][Governor].level);
    notify(Playernum, Governor, buf);
    return;
}
if(!control(Playernum, Governor, Stars[Dir[Playernum-1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
}
if (!enufAP(Playernum,Governor,Stars[Dir[Playernum-1][Governor].snum]->AP[Playernum-1], APcount)) {
    return;
}

getplanet(&p,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);

if(argn < 2) {
    sprintf(buf, "Current investment : %d    Technology production/update: %.3f\n",
	    p->info[Playernum-1].tech_invest,
	    tech_prod((int)(p->info[Playernum-1].tech_invest),
		      (int)(p->info[Playernum-1].popn)));
    notify(Playernum, Governor, buf);
    free(p);
    return;
}
invest=atoi(args[1]);

if (invest < 0) {
    sprintf(buf,"Illegal value.\n");
    notify(Playernum, Governor, buf);
    free(p);
    return;
}

p->info[Playernum-1].tech_invest = invest;

putplanet(p,Dir[Playernum-1][Governor].snum,Dir[Playernum-1][Governor].pnum);

deductAPs(Playernum, Governor, APcount, Dir[Playernum-1][Governor].snum, 0);

sprintf(buf,"   New (ideal) tech production: %.3f (this planet)\n", 
	tech_prod((int)(p->info[Playernum-1].tech_invest),
		  (int)(p->info[Playernum-1].popn)));
notify(Playernum, Governor, buf);

free(p);	
}

double tech_prod(int investment, int popn)
{
    double scale;

    scale = (double)popn/10000.;
    return (TECH_INVEST * log10( (double)investment * scale+1.0));
}





