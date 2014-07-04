/* shlmisc.c function prototypes */

extern char *Ship(shiptype *s);
extern void grant(int, int, int);
extern void governors(int, int, int);
extern void do_revoke(racetype *, int, int);
extern int authorized(int, shiptype *);
extern int start_shiplist(int, int, char *);
extern int do_shiplist(shiptype **, int *);
extern int in_list(int, char *, shiptype *, int *);
extern void fix(int, int);
extern int match(char *, char *);
extern void DontOwnErr(int, int, int);
extern int enufAP(int, int, unsigned short, int);
extern int Getracenum(char *, char *, int *, int *);
extern int GetPlayer(char *);
extern void allocateAPs(int, int, int);
extern void deductAPs(int, int, int, int, int);
extern void list(int, int);
extern double morale_factor(double);
#if DEBUG
extern char *DEBUGmalloc(int, char *, int);
extern void DEBUGfree(char *);
extern char *DEBUGrealloc(char *, int, char *, int);
extern void DEBUGcheck(int, int);
extern void DEBUGreset(int, int);
#endif

