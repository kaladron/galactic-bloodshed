/*
 * guardian.c - makes sure your server keeps running.
 *
 */

#include <stdio.h>
#include <signal.h>

char *prog;

main(argc,argv)
 int argc;
 char *argv[];
{
    int pid;
    
    (void)sigblock(SIGHUP);	/* ignore signal when owner logs out */

    prog = *argv++;		/* remember guardian name */
    fprintf(stderr,"%s: ",prog);
    while (1) {
	switch (pid=fork()) {
	  case -1:		/* error! */
	    perror("can't fork a child");
	    sleep(600);		/* 10 min */
	    break;
	  case 0:		/* child */
	    execvp(argv[0],argv);
	    perror("can't exec child");
	    exit(1);
	    break;
	  default:		/* parent */
	    fprintf(stderr,"starting %s with pid [%d]\n",argv[0],pid);
	    wait(0);		/* wait for the little brat */
	    break;
	}
	fprintf(stderr,"%s: re",prog);
	sleep(60); /* wait a minute then try to start it up again */
    }
}
