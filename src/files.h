/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, { *
 * smq@ucscb.ucsc.edu, mods by people in GB.c, enroll.dat.
 * Restrictions in GB.c.
 */

#define PATH(file) "/tmp/GB/" #file
#define DIRPATH(dir, file) "/tmp/GB/" dir #file
#define DATA(file) DIRPATH("Data/", file)
#define NEWS(file) DIRPATH("News/", file)
#define TELE(file) DIRPATH("Tele/", file)

#define PATHLEN 200 /* length of file paths to the game.. */

#define CUTE_MESSAGE "\nThe Galactic News\n\n"
#define DECLARATION 0 /* news file index */
#define TRANSFER 1
#define COMBAT 2
#define ANNOUNCE 3

#define DATADIR Files[0]
#define DOCSDIR Files[1]
#define EXAM_FL Files[2]
#define ENROLL_FL Files[3]
#define STARDATAFL Files[4]
#define SECTORDATAFL Files[5]
#define PLANETDATAFL Files[6]
#define RACEDATAFL Files[7]
#define BLOCKDATAFL Files[8]
#define SHIPDATAFL Files[9]
#define SHIPFREEDATAFL Files[10]
#define DUMMYFL Files[11]
#define PLAYERDATAFL Files[12]
#define TELEGRAMDIR Files[13]
#define TELEGRAMFL Files[14]
#define POWFL Files[15]
#define NEWSDIR Files[16]
#define DECLARATIONFL Files[17]
#define TRANSFERFL Files[18]
#define COMBATFL Files[19]
#define ANNOUNCEFL Files[20]
#define COMMODDATAFL Files[21]
#define COMMODFREEDATAFL Files[22]
#define UPDATEFL Files[23]
#define SEGMENTFL Files[24]

#define PLANETLIST PATH(planet.list)
#define STARLIST PATH(star.list)

#define NOGOFL PATH(nogo)
#define ADDRESSFL PATH(Addresses)

extern char *Files[];
