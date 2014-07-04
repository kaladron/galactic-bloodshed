/* move.c function prototypes */

extern void arm(int, int, int, int);
extern void move_popn(int, int, int);
extern void walk(int, int, int);
extern int get_move(char, int, int, int *, int *, planettype *);
extern void mech_defend(int, int, int *, int, planettype *, int, int, sectortype *,
		 int, int, sectortype *);
extern void mech_attack_people(shiptype *, int *, int *, racetype *, racetype *,
			sectortype *, int, int, int, char *, char *);
extern void people_attack_mech(shiptype *, int, int, racetype *, racetype *,
			sectortype *, int, int, char *, char *);
extern void ground_attack(racetype *, racetype *, int *, int, unsigned short *,
		   unsigned short *, unsigned int, unsigned int,
		   double, double, double *, double *, int *, int *, int *);

