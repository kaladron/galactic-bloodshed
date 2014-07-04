/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *  disk input/output routines
 *
 *  Fileread(p, num, file, posn, routine); -- generic file read
 *  Filewrite(p, num, file, posn, routine); -- generic file write
 *
 */

#include <errno.h>
#include <signal.h>
#include <sys/file.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "files.h"
#include "buffers.h"

int sys_nerr;

extern char *sys_errlist[];
extern int errno;

void Fileread(int, char *, int, int);
void Filewrite(int, char *, int, int);

void Fileread(int fd, char *p, int num, int posn)
{
 int n2;

 if (lseek(fd, posn, L_SET) < 0) {
	perror("Fileread 1");
	return;
 }
 if ((n2=read(fd, p, num))!=num) {
	perror("Fileread 2");
 }
}

void Filewrite(int fd, char *p, int num, int posn)
{
    int n2;

    if (lseek(fd, posn, L_SET) < 0) {
	perror("Filewrite 1");
	return;
    }

    if ((n2=write(fd,p,num))!=num) {
	perror("Filewrite 2");
	return;
    }
}



