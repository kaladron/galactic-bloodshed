/* 
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * scales used in production efficiency etc.
 * input both: int 0-100
 * output both: float 0.0 - 1.0 (logscaleOB 0.5 - .95)
 */

#include "GB_copyright.h"
#include <math.h>

double logscale(int);

double logscale(int x)
{
/* return (x+5.0) / (x+10.0); */
 return log10((double)x+1.0)/2.0;
}

