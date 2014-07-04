/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * zoom.c -- zoom in or out for orbit display
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "buffers.h"

void zoom(int, int, int);
#include "GB_server.p"

void zoom(int Playernum, int Governor, int APcount)
{
 double num, denom;
 int i;

 i = (Dir[Playernum-1][Governor].level==LEVEL_UNIV);

 if(argn > 1) {
     if (sscanf(args[1], "%lf/%lf", &num, &denom) == 2) {
	 /* num/denom format */
	 if (denom==0.0) {
	     sprintf(buf,"Illegal denominator value.\n");
	     notify(Playernum, Governor, buf);
	 } else
	     Dir[Playernum-1][Governor].zoom[i] = num / denom;
     } else {
	 /* one number */
	 Dir[Playernum-1][Governor].zoom[i] = num;
     }
 }

 sprintf(buf,"Zoom value %g, lastx = %g, lasty = %g.\n",Dir[Playernum-1][Governor].zoom[i],
	 Dir[Playernum-1][Governor].lastx[i],Dir[Playernum-1][Governor].lasty[i]);
 notify(Playernum, Governor, buf);
}
