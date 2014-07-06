/* shootblast.c function prototypes */

extern int shoot_ship_to_ship(shiptype *, shiptype *, int, int, int, char *,
                              char *);
#ifdef DEFENSE
extern int shoot_planet_to_ship(racetype *, planettype *, shiptype *, int,
                                char *, char *);
#endif
extern int shoot_ship_to_planet(shiptype *, planettype *, int, int, int, int,
                                int, int, char *, char *);
extern int do_radiation(shiptype *, double, int, int, char *, char *);
extern int do_damage(int, shiptype *, double, int, int, int, int, double,
                     char *, char *);
extern void ship_disposition(shiptype *, int *, int *, int *);
extern int CEW_hit(double, int);
extern int Num_hits(double, int, int, double, int, int, int, int, int, int, int,
                    int);
extern int hit_odds(double, int *, double, int, int, int, int, int, int, int,
                    int);
extern int cew_hit_odds(double, int);
extern double gun_range(racetype *, shiptype *, int);
extern double tele_range(int, double);
extern int current_caliber(shiptype *);
extern void do_critical_hits(int, shiptype *, int *, int *, int, char *);
extern void do_collateral(shiptype *, int, int *, int *, int *, int *);
extern int getdefense(shiptype *);
extern double p_factor(double, double);
extern int planet_guns(int);
extern void mutate_sector(sectortype *);
