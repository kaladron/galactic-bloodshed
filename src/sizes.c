/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 */

#include "GB_copyright.h"
#define EXTERN
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"

main() {
 printf(" size startype is %d\n",sizeof(startype));
 printf(" size planettype is %d\n",sizeof(planettype));
 printf(" size shiptype is %d\n\n",sizeof(shiptype));

 printf(" size racetype is %d\n",sizeof(racetype));
 printf(" size placetype is %d\n",sizeof(placetype));
 printf(" size struct plinfo is %d\n\n",sizeof(struct plinfo));
 printf(" size struct power [15] is %d\n\n",sizeof(Power));

 printf(" size sectortype is %d\n",sizeof(sectortype));

 printf(" size long = %d\n",sizeof(long) );
 printf(" size int = %d\n",sizeof(int) );
 printf(" size short = %d\n",sizeof(short) );
 printf(" size double = %d\n",sizeof(double) );
 printf(" size float = %d\n",sizeof(float) );
}
