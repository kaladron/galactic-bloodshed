/* doplanet.c function prototypes */

extern int doplanet(int, planettype *, int);
extern int moveship_onplanet(shiptype *, planettype *);
extern void terraform(shiptype *, planettype *);
extern void plow(shiptype *, planettype *);
extern void do_dome(shiptype *, planettype *);
extern void do_quarry(shiptype *, planettype *);
extern void do_berserker(shiptype *, planettype *);
extern void do_recover(planettype *, int, int);
extern double est_production(sectortype *);

