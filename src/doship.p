/* doship.c function prototypes */

extern void doship(shiptype *, int);
extern void domass(shiptype *);
extern void doown(shiptype *);
extern void domissile(shiptype *);
extern void domine(int, int);
extern void doabm(shiptype *);
extern void do_repair(shiptype *);
extern void do_habitat(shiptype *);
extern void do_pod(shiptype *);
extern int infect_planet(int, int, int);
extern void do_meta_infect(int, planettype *);
extern void do_canister(shiptype *);
extern void do_greenhouse(shiptype *);
extern void do_mirror(shiptype *);
extern void do_god(shiptype *);
extern void do_ap(shiptype *);
extern double crew_factor(shiptype *);
extern double ap_planet_factor(planettype *);
extern void do_oap(shiptype *);
extern int do_weapon_plant(shiptype *);


