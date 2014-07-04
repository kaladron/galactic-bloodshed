/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * Galactic Bloodshed (Robert Chansky, smq@ucscb.ucsc.edu)
 * victory.c
 */
#include <errno.h>
#include <time.h>
#include <strings.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

extern int errno;

void victory(int, int, int);
void create_victory_list(struct vic [MAXPLAYERS]);
int victory_sort(struct vic *, struct vic *);
#include "GB_server.p"

void victory(int Playernum, int Governor, int APcount)
{
struct vic vic[MAXPLAYERS];
racetype *Race;
int i;
int count;
int god = 0;

/*
#ifndef VICTORY
notify(Playernum, Governor, "Victory conditions disabled.\n");
return;
#endif
*/
count = (argn > 1)?atoi(args[1]):Num_races;
if (count > Num_races)
    count = Num_races;

create_victory_list (vic);

Race = races[Playernum-1];
if (Race->God)
    god = 1;

sprintf(buf, "----==== PLAYER RANKINGS ====----\n");
notify(Playernum, Governor, buf);
sprintf(buf, "%-4.4s %-15.15s %8s\n", 
	"No.", "Name", (god ? "Score" : ""));
notify(Playernum, Governor, buf);
for (i = 0; i < count; i++) {
    if (god)
	sprintf(buf, "%2d %c [%2d] %-15.15s %5d  %6.2f %3d %s %s\n",
		i+1, vic[i].Thing ? 'M' : ' ', vic[i].racenum,
		vic[i].name, vic[i].rawscore,
		vic[i].tech, vic[i].IQ, races[vic[i].racenum-1]->password,
		races[vic[i].racenum-1]->governor[0].password);
    else
	sprintf(buf, "%2d   [%2d] %-15.15s\n", i+1,
		vic[i].racenum, vic[i].name);
    notify(Playernum, Governor, buf);
}
}

void create_victory_list(struct vic vic[MAXPLAYERS])
{
racetype *vic_races[MAXPLAYERS];
int i;

for (i = 1; i <= Num_races; i++) {
    vic_races[i-1] = races[i-1];
    vic[i-1].no_count = 0;
}
for (i = 1; i <= Num_races; i++) {
    vic[i-1].racenum = i;
    strcpy(vic[i-1].name, vic_races[i-1]->name);
 		vic[i-1].rawscore = vic_races[i-1]->victory_score; 
/*    vic[i-1].rawscore = vic_races[i-1]->morale; */
    vic[i-1].tech = vic_races[i-1]->tech;
    vic[i-1].Thing = vic_races[i-1]->Metamorph;
    vic[i-1].IQ = vic_races[i-1]->IQ;
    if (vic_races[i-1]->God || vic_races[i-1]->Guest ||
	vic_races[i-1]->dissolved)
	vic[i-1].no_count = 1;
}
qsort(vic, Num_races, sizeof(struct vic), victory_sort);
}

int victory_sort(struct vic *a, struct vic *b)
{
    if (a->no_count)
	return (1);
    else if (b->no_count)
	return (-1);
    else
	return (b->rawscore - a->rawscore);
}
