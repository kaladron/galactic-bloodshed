/*
O * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB.c, enroll.dat.
 * Restrictions in GB.c.
 */

char Shipltrs[]={ 'p','s','X','D','B','I','C','d','f','e','H','S','O','c',
	't','!','+','M',
	'=','\\','-','a','g','h','v','V','@','l','w',':','G','F',
	'T',';','Z','[','^','P', 'q', 'K', 'Y', 'W', 'J', '&', 'R', 'b', 'L'};

/* table for [ABIL_BUILD]. (bd). sum the numbers to get the correct value.
/*      1 To allow it to be built on a planet.
/*      2 For building by warships (d, B, C,..). Currently only for Space Probe.
/*	  Mines used to be this way too. Built in hanger of building ship.
/*      4 For building by Shuttles, Cargo ship, Habitats, etc.
/*        Also forces construction on the outside of the ship. Not in hanger.
/*      8 For building in Factories. Built on planet, or in hanger of carrying
/*        ship (Habitat).
/*     16 For building in Habitats. Used by Pods for instance. Also used by
/*        Factories. Built inside Habitat. */

/* table for [ABIL_CONSTRUCT]. (cn). sum the numbers to get the correct value.
/*      1 To allow it to build like a planet.
/*      2 For building like warships (d, B, C,..).
/*      4 For building like Shuttles, Cargo ship, Habitats, etc.
/*      8 For building like Factories.
/*     16 For building like Habitats. */

/* Changes here to use the new build routine using above tables.  Maarten
/* Also changed:
/*   - Pods, Factories, Weapons Plants, Terraforming Devices,
/*     Orbital Mind Control Lasers and Government Centers can 
/*     be built inside Habitats.
/*   - Probes, and other type 2 ships (currently none), are now built inside
/*     ships, requiring hanger space. This gives more incentive to keep some
/*     hanger space in the big warships.
/*   - The big space stations (Habitats, Stations, and Orbital Assault
/*     Platforms) can now build Probes as well.

 /*   - Habitats and Stations had their ability to use a crystal mount removed.
/*     Since they cannot use it in any way, it was rather useless. It only
/*     confused the required resources to build the ship, though this has been
/*     taken care of too.
/*   - Orbital Mind Control Lasers having 10 guns of caliber 0 seemed strange.
/*     Now 0 guns. Also removed the 100 destruct carrying capacity. Added 25
/*     cargo space so it can repair itself. */


long Shipdata[NUMSTYPES][NUMABILS] = {
   /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp dm  bd   cn mod las cew clk god prg port rep pay */
/*SPd*/   0,   0,    0,   0,   0,   0,  0,    20,    1,  0,   1,  0, 0, 1, 0, 2,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/*Shu*/  10,  25,    2,   2,   1,   1,  0,    20,   10,  0,   2,  0, 0, 1, 0, 4,  0,  8,  4, 1,  0,  0,  0,  0,   1,  0,  0,  1,
/*Car*/ 250, 600,  200, 800,  30,   3,  2,  1000,   30,  5,  30,  1, 1, 0, 0, 4, 50, 20,  2, 1,  1,  1,  0,  0,   1,  0,  1,  1,
/*Drn*/ 300, 500,   10, 500,  60,   3,  2,   500,   60, 10,  40,  1, 1, 1, 0, 6, 50,  8,  2, 1,  1,  1,  0,  0,   1,  0,  0,  1,
/*BB */ 200, 235,   10, 400,  30,   3,  2,   200,   30,  7,  20,  1, 1, 1, 0, 6, 50,  8,  2, 1,  1,  1,  0,  0,   1,  0,  0,  1,
/*Int*/ 150, 110,    5, 120,  20,   2,  2,   200,   20,  3,  15,  1, 1, 1, 0, 6, 50,  8,  2, 1,  1,  1,  0,  0,   1,  0,  0,  1,
/*CA */ 150, 165,    5, 300,  20,   3,  2,   120,   20,  5,  10,  1, 1, 1, 0, 6, 50,  8,  2, 1,  1,  1,  0,  0,   1,  0,  0,  1,
/*DD */ 100, 110,    5, 120,  15,   2,  2,    80,   15,  3,   5,  1, 1, 1, 0, 6, 50,  8,  2, 1,  1,  1,  0,  0,   1,  0,  0,  1,
/*FF */ 100,   0,    0,  40,  20,   2,  1,    10,    1,  2,   1,  1, 1, 1, 0, 9,  0,  8,  2, 1,  1,  1,  0,  0,   1,  0,  1,  1,
/*Exp*/  40,  10,    0,  15,   5,   2,  0,    35,    5,  1,   2,  1, 1, 1, 0, 6,  0,  8,  0, 1,  1,  0,  0,  0,   1,  0,  0,  1,
/*Hab*/ 100,5000,   10, 500,  20,   2,  1,  2000,  2000, 3,  50,  0, 0, 0, 1, 4, 75, 20, 18, 1,  0,  0,  0,  0,   1,  1,  1,  1,
/*Stn*/ 100,5000,   10, 250,  20,   2,  0,  2000,   50,  1,  10,  0, 0, 0, 0, 4, 75, 20,  6, 1,  0,  0,  0,  0,   1,  1,  1,  1,
/*OSP*/ 200,1400,   20,1000,  50,   3,  2,  2000,  200,  5,  40,  1, 1, 0, 0, 4, 75, 20,  6, 1,  1,  1,  0,  0,   1,  0,  1,  1,
/*Crg*/ 100,1000,    5,1000,  10,   1,  0,  1000,  100,  2,  10,  1, 1, 1, 0, 4,  0,  8,  4, 1,  0,  0,  0,  0,   1,  0,  0,  1,
/*Tnk*/ 100, 200,    5, 200,  10,   1,  0,  5000,   10,  2,  10,  1, 1, 1, 0, 4,  0,  8,  2, 1,  0,  0,  0,  0,   1,  0,  0,  1,
/*GOD*/9999,20000,1000,20000, 1000, 3,  3, 20000, 1000,100,  10,  1, 1, 1, 0, 9,  0,  1,  6, 1,  1,  1,  0,  1,   1,  1,  1,  0,
/*SMn*/  50,   0,    0,  25,   0,   0,  0,    20,    0,  1,  30,  0, 0, 1, 1, 2,  0,  8,  0, 1,  0,  0,  0,  0,   1,  0,  0,  0,
   /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp dm  bd  cn mod las cew clk god prg port*/
/*mir*/ 100, 200,    0,  10,   1,   1,  0,    20,    5,  0, 100,  0, 0, 0, 0, 2, 75, 20,  0, 0,  0,  0,  0,  0,   1,  0,  1,  1,
/*Stc*/  50,   0,    0,   0,   0,   0,  0,    20,    2,  0,  20,  1, 1, 1, 0, 4,  0,  8,  0, 1,  0,  0,  0,  0,   1,  0,  0,  1,
/*Tsc*/   5,   0,    0,   0,   0,   0,  0,     0,    2,  0,   2,  0, 0, 1, 0, 0,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/*T-R*/ 200,   0,    0,   0,   0,   0,  0,  1000,    5,  0,  20,  1, 0, 1, 1, 2,  0,  1,  0, 0,  0,  0,  0,  0,   0,  0,  1,  0,
/*APr*/  80,   0,    0,   0,   0,   0,  0,   200,   10,  1,  20,  0, 0, 1, 1, 0,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/* CD*/  40,   0,    0,   0,   0,   0,  0,     1,    0,  0,  10,  0, 0, 1, 1, 1,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/*Grn*/  40,   0,    0,   0,   0,   0,  0,     1,    0,  0,  10,  0, 0, 1, 0, 1,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/*VN */  80,  20,    0,   0,   0,   0,  0,    50,    0,  1, 100,  0, 0, 1, 0, 4,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  0,  0,
/*Bers*/999,  50,    0, 500,  40,   3,  2,  1000,    0, 15, 100,  0, 0, 1, 1, 6,  0,  1,  0, 0,  0,  0,  0,  1,   1,  0,  1,  0,
/*Gov*/   0, 500,    0, 100,  10,   1,  0,  1000,   10, 20, 500,  0, 0, 1, 0, 0, 75, 17,  0, 0,  0,  0,  0,  0,   1,  1,  1,  0,
/*OMCL*/350,  25,    0,   0,   0,   0,  0,   100,    2,  1,  50,  0, 0, 1, 1, 4,  0, 17,  0, 0,  0,  0,  0,  0,   0,  0,  0,  0,
/*TWC*/   0,   0,    0,   0,   0,   0,  0,    20,    0,  0,   5,  0, 0, 1, 0, 4,  0,  1,  0, 0,  0,  0,  0,  0,   1,  0,  0,  0,
/*Prb*/ 150,   0,    0,   0,   0,   0,  0,    20,    0,  0,  10,  0, 0, 1, 0, 9,  0, 19,  0, 0,  0,  0,  0,  0,   1,  0,  0,  0,
   /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp dm  bd  cn mod las cew clk god prg port*/
/*GRL */100,  50,    0, 120,  20,   1,  0,     0,   40,  3,  30,  0, 0, 1, 1, 0, 75,  1,  0, 1,  0,  1,  0,  0,   1,  0,  1,  1,
/*Fac*/   0,  50,    0,   0,   0,   0,  0,     0,   20,  0,  20,  0, 0, 1, 1, 0, 75, 17,  8, 0,  0,  0,  0,  0,   1,  0,  1,  1,
/*TFD*/  50,  40,    5,   0,   0,   0,  0,   200,   20,  1,  20,  1, 1, 1, 1, 4,  0, 17,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1,
/*BCC*/9999, 200,    0,  50,   0,   3,  0,     0,    0, 10,   3,  0, 0, 1, 1, 0,  0,  1,  0, 0,  0,  0,  0,  1,   0,  0,  1,  0,
/*BAf*/9999,1000,    0,1000,   0,   0,  0,  1000,     0, 10,  8,  0, 0, 1, 1, 0,  0,  1,  0, 0,  0,  0,  0,  1,   0,  0,  1,  0,
/*TD */ 200,1000,    0,1000,   0,   0,  0,  1000,   100,  0, 300, 0, 0, 1, 1, 0, 50,  1,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/*Mis*/  50,   0,    0,  10,   0,   0,  0,     5,     0,  0,   5, 0, 0, 0, 1, 6,  0,  8,  0, 1,  0,  0,  0,  0,   1,  0,  0,  0,
/*PDN*/ 200,  50,    0, 500,  20,   3,  0,     0,    50, 10, 100, 0, 0, 1, 1, 0, 75,  1,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1,
   /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp dm  bd  cn mod las cew clk god prg port*/
/*Qua*/   0,   0,    0,   0,  0,   0,  0,   200,    50,  1,  10,  0, 0, 1, 1, 0,  0,  1,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1,
/*Plo*/   5,   0,    0,   0,  0,   0,  0,   200,    10,  1,  10,  0, 0, 1, 1, 0,  0,  1,  0, 1,  0,  0,  0,  0,   1,  0,  0,  0,
/*Dom*/  10, 100,    0,   0,  0,   0,  0,     0,    20,  1,  10,  0, 0, 1, 1, 0,  0,  1,  0, 1,  0,  0,  0,  0,   1,  0,  1,  0,
/*Wea*/   0, 500,    0,   0,  0,   0,  0,   500,    20,  5,  20,  0, 0, 1, 0, 0, 75, 17,  0, 0,  0,  0,  0,  0,   1,  0,  1,  0,
/*Port*/  0,   0,    0,   0,  0,   0,  0,     0,   100,  3,  50,  0, 0, 1, 0, 0, 75,  1,  0, 1,  0,  0,  0,  0,   1,  1,  1,  1,
/*ABM*/ 100,   5,    0,  50,  5,   1,  0,     0,     5,  5,  50,  0, 0, 1, 1, 0, 50,  1,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1,
   /*  tech  carg  bay  dest guns prim sec fuelcap  crw arm  cst mt jp ld sw sp dm  bd  cn mod las cew clk god prg port*/
/*AFV*/  50,   5,    0,  20,  2,   1,  0,    20,     1,  2,  20,  0, 0, 0, 0, 0,  0,  8,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1,
/*Bun*/  10, 100,   20, 100,  0,   0,  0,   100,   100, 15, 100,  0, 0, 0, 0, 0, 50,  1,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1,
/*Lnd*/ 150, 100,   10, 200, 10,   3,  0,   100,   500,  7,  50,  1, 1, 1, 0, 2, 50,  8,  0, 1,  0,  0,  0,  0,   1,  0,  1,  1
};

char *Shipnames[NUMSTYPES] = {
   "Spore pod",
   "Shuttle",
   "Carrier",
   "Dreadnaught",
   "Battleship",
   "Interceptor",
   "Cruiser",
   "Destroyer",
   "Fighter Group",
   "Explorer",
   "Habitat",
   "Station",
   "Ob Asst Pltfrm",
   "Cargo Ship",
   "Tanker",
   "GODSHIP",
   "Space Mine",
   "Space Mirror",
   "Space Telescope",
   "Ground Telescope",
   "* T-R beam",
   "Atmosph Processor",
   "Dust Canister",
   "Greenhouse Gases",
   "V.Neumann Machine",
   "Berserker",
   "Govrnmnt. Center",
   "Mind Control Lsr",
   "Tox Waste Canistr",
   "Space Probe",
   "Gamma Ray Laser",
   "Factory",
   "Terraform Device",
   "Bers Cntrl Center",
   "Bers Autofac",
   "AVPM Transporter",
   "Missile",
   "Planet Def Net",
   "Quarry",
   "Space Plow",
   "Dome",
   "Weapons Plant",
   "Space Port",
   "ABM Battery",
   "Mech",
   "Bunker",
   "Lander"
   };
