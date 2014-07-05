/*
build.c function prototypes
*/

extern void upgrade(int, int, int);
extern void make_mod(int, int, int, int);
extern void build(int, int, int);
extern int getcount(int, char *);
extern int can_build_at_planet(int, int, startype *, planettype *);
extern int get_build_type(char *);
extern int can_build_this(int, racetype *, char *);
extern int can_build_on_ship(int, racetype *, shiptype *, char *);
extern int can_build_on_sector(int, racetype *, planettype *, sectortype *, int,
                               int, char *);
extern int build_at_ship(int, int, racetype *, shiptype *, int *, int *);
extern void autoload_at_planet(int, shiptype *, planettype *, sectortype *,
                               int *, double *);
extern void autoload_at_ship(int, shiptype *, shiptype *, int *, double *);
extern void initialize_new_ship(int, int, racetype *, shiptype *, double, int);
extern void create_ship_by_planet(int, int, racetype *, shiptype *,
                                  planettype *, int, int, int, int);
extern void create_ship_by_ship(int, int, racetype *, int, startype *,
                                planettype *, shiptype *, shiptype *);
extern double getmass(shiptype *);
extern int ship_size(shiptype *);
extern double cost(shiptype *);
extern void system_cost(double *, double *, int, int);
extern double complexity(shiptype *);
extern void Getship(shiptype *, int, racetype *);
extern void Getfactship(shiptype *, shiptype *);
extern int Shipcost(int, racetype *);
extern void sell(int, int, int);
extern void bid(int, int, int);
extern int shipping_cost(int, int, double *, int);
