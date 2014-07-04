/* GB_server.c function prototypes */

extern void notify_race(int, char *);
extern int notify(int, int, char *);
extern void d_think(int, int, char *);
extern void d_broadcast(int, int, char *);
extern void d_shout(int, int, char *);
extern void d_announce(int, int, int, char *);
extern void do_next_thing(void);
extern void load_race_data(void);
extern void load_star_data(void);
extern void GB_time(int, int);
extern void GB_schedule(int, int);
extern void check_for_telegrams(int, int);
extern void kill_ship(int, shiptype *);
extern void compute_power_blocks(void);
extern void insert_sh_univ(struct stardata *, shiptype *);
extern void insert_sh_star(startype *, shiptype *);
extern void insert_sh_plan(planettype *, shiptype *);
extern void insert_sh_ship(shiptype *, shiptype *);
extern void remove_sh_star(shiptype *);
extern void remove_sh_plan(shiptype *);
extern void remove_sh_ship(shiptype *, shiptype *);
extern double GetComplexity(int);
extern int ShipCompare(int *, int *);
extern void SortShips(void);
extern void warn_race(int, char *);
extern void warn(int, int, char *);
extern void warn_star(int, int, int, char *);
extern void notify_star(int, int, int, int, char *);
extern void post_star(char *, int, int);
extern void adjust_morale(racetype *, racetype *, int);

