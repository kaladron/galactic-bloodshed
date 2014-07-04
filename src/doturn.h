/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB.c, enroll.dat.
 * Restrictions in GB.c.
 * doturn.h -- various turn things we are keeping track of.
 * Galactic Bloodshed by Robert Chansky
 */


struct stinfo {
	short temp_add;	/* addition to temperature to each planet */
	unsigned char Thing_add;	
			/* new Thing colony on this planet */
	unsigned char inhab;		/* explored by anybody */
	unsigned char intimidated;	/* assault platform is here */
};

EXTERN struct stinfo Stinfo[NUMSTARS][MAXPLANETS];

struct vnbrain {
	unsigned short Total_mad;	/* total # of VN's destroyed so far */
	unsigned char Most_mad;	/* player most mad at */
};

EXTERN struct vnbrain VN_brain;

struct sectinfo {
	char explored;		/* sector has been explored */
	unsigned char VN;	/* this sector has a VN */
	unsigned char done;	/* this sector has been updated */
};

EXTERN struct sectinfo Sectinfo[MAX_X][MAX_Y];
