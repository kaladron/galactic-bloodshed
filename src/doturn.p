/* doturn.c function prototypes */

extern void do_turn(int);
extern int APadd(int, int, racetype *);
extern int governed(racetype *);
extern void fix_stability(startype *);
extern void do_reset(int);
extern void handle_victory(void);
extern void make_discoveries(racetype *);
#ifdef MARKET
extern void maintain(racetype *, int, int);
#endif
extern int attack_planet(shiptype *);
extern void output_ground_attacks(void);
extern int planet_points(planettype *);

