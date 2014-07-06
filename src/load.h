/* load.c function prototypes */

extern void load(int, int, int, int);
extern void jettison(int, int, int);
extern int jettison_check(int, int, int, int);
extern void dump(int, int, int);
extern void transfer(int, int, int);
extern void mount(int, int, int, int);
extern void use_fuel(shiptype *, double);
extern void use_destruct(shiptype *, int);
extern void use_resource(shiptype *, int);
extern void use_popn(shiptype *, int, double);
extern void rcv_fuel(shiptype *, double);
extern void rcv_resource(shiptype *, int);
extern void rcv_destruct(shiptype *, int);
extern void rcv_popn(shiptype *, int, double);
extern void rcv_troops(shiptype *, int, double);
extern void do_transporter(racetype *, int, shiptype *);
extern int landed_on(shiptype *, int);
extern void unload_onto_alien_sector(int, int, planettype *, shiptype *,
                                     sectortype *, int, int);
