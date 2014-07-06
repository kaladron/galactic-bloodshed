/* files_shl.c function prototypes */

#include "power.h"

extern void close_file(int);
extern void open_data_files(void);
extern void close_data_files(void);
extern void openstardata(int *);
extern void openshdata(int *);
extern void opencommoddata(int *);
extern void openpdata(int *);
extern void opensectdata(int *);
extern void openracedata(int *);
extern void getsdata(struct stardata *S);
#ifdef DEBUG
extern void DEBUGgetrace(racetype **, int, char *, int);
extern void DEBUGgetstar(startype **, int, char *, int);
extern void DEBUGgetplanet(planettype **, int, int, char *, int);
extern int DEBUGgetship(shiptype **, int, char *, int);
extern int DEBUGgetcommod(commodtype **, int, char *, int);
#else
extern void getrace(racetype **, int);
extern void getstar(startype **, int);
extern void getplanet(planettype **, int, int);
extern int getship(shiptype **, int);
extern int getcommod(commodtype **, int);
#endif
extern void getsector(sectortype **, planettype *, int, int);
extern void getsmap(sectortype *, planettype *);
extern int getdeadship(void);
extern int getdeadcommod(void);
extern void putsdata(struct stardata *);
extern void putrace(racetype *);
extern void putstar(startype *, int);
extern void putplanet(planettype *, int, int);
extern void putsector(sectortype *, planettype *, int, int);
extern void putsmap(sectortype *, planettype *);
extern void putship(shiptype *);
extern void putcommod(commodtype *, int);
extern int Numraces(void);
extern int Numships(void);
extern int Numcommods(void);
extern int Newslength(int);
extern void clr_shipfree(void);
extern void clr_commodfree(void);
extern void makeshipdead(int);
extern void makecommoddead(int);
extern void Putpower(struct power[MAXPLAYERS]);
extern void Getpower(struct power[MAXPLAYERS]);
extern void Putblock(struct block[MAXPLAYERS]);
extern void Getblock(struct block[MAXPLAYERS]);
