/* fire.c function prototypes */

extern void fire(int, int, int, int);
extern void bombard(int, int, int);
extern void defend(int, int, int);
extern void detonate(int, int, int);
extern int retal_strength(shiptype *);
extern int adjacent(int, int, int, int, planettype *);
extern int landed(shiptype *);
extern void check_overload(shiptype *, int, int *);
extern void check_retal_strength(shiptype *, int *);
extern int laser_on(shiptype *);

