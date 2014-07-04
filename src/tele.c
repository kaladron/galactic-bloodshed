/***********************************************
 * #ident  "@(#)tele.c	1.13 12/3/93 "
 * tele.c
 *
 * Created: ??
 * Author:  Robert Chansky
 *
 * Version: 1.10 09:06:00
 *
 * Contains: purge()
 *           post()
 *           push_telegram_race()
 *           push_telegram()
 *           teleg_read()
 *           news_read()
 *
 ***********************************************/

#define EXTERN extern
#include "GB_copyright.h"
#include "vars.h"
#include "files.h"
#include "races.h"
#include "buffers.h"
#include "power.h"
#include "ships.h"
#ifdef AIX
#include <time.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>


static long            tm;
static FILE            *teleg_read_fd;
static char            telegram_file[PATHLEN];
static struct 			stat telestat;


/*
 * Prototypes
 */
EXTERN void            purge(void);
EXTERN void            post(char *, int);
EXTERN void            push_telegram_race(int, char *);
EXTERN void            push_telegram(int, int, char *);
EXTERN void            teleg_read(int, int);
EXTERN void            news_read(int, int, int);
#include "proto.h"

static struct tm      *current_tm;	/* for watching for next update */

/*
 * purge:
 * 
 * arguments: none
 * 
 * called by: process_commands
 * 
 * description:  Used to purge the News files.
 * 
 */
void 
purge(void)
{
	fclose(fopen(DECLARATIONFL, "w+"));
	newslength[0] = 0;
	fclose(fopen(COMBATFL, "w+"));
	newslength[1] = 0;
	fclose(fopen(ANNOUNCEFL, "w+"));
	newslength[2] = 0;
	fclose(fopen(TRANSFERFL, "w+"));
	newslength[3] = 0;
}

/*
 * post:
 * 
 * arguments: msg  The actual message type Type of message.  Valid types are
 * DECLARATION, TRANSFER, COMBAT and ANNOUNCE.
 * 
 * called by:  fire, name, declare, dock, land, dissolve, doship, doturn
 * 
 * description: does the acutal posting of messages to the news files
 * 
 */
void 
post(char *msg, int type)
{
	char            telefl[100];
	char            pbuf[1024];	/* this is needed, don't use global
					 * pointer! */
	FILE           *news_fd;
	char           *p;

	bzero((char *)telefl, sizeof(telefl));
	switch (type) {
	case DECLARATION:
		sprintf(telefl, "%s", DECLARATIONFL);
		break;
	case TRANSFER:
		sprintf(telefl, "%s", TRANSFERFL);
		break;
	case COMBAT:
		sprintf(telefl, "%s", COMBATFL);
		break;
	case ANNOUNCE:
		sprintf(telefl, "%s", ANNOUNCEFL);
		break;
	default:
		return;
	}

	/* look for special symbols */
	for (p = msg; *p; p++) {
		if (*p == ';')
			*p = '\n';
		else if (*p == '|')
			*p = '\t';
	}

	if ((news_fd = fopen(telefl, "a")) == NULL) {
		return;
	} else {
		tm = time(0);
		current_tm = localtime(&tm);
		sprintf(pbuf, "%2d/%2d %02d:%02d:%02d %s",
			current_tm->tm_mon + 1, current_tm->tm_mday, current_tm->tm_hour,
			current_tm->tm_min, current_tm->tm_sec, msg);
		fprintf(news_fd, "%s", pbuf);
		fclose(news_fd);
		newslength[type] += strlen(pbuf);
	}
}

/*
 * push_telegram_race:
 * 
 * arguments: recpient msg
 * 
 * called by:
 * 
 * description:  Sends a message to everyone in the race
 * 
 */
void
push_telegram_race(int recpient, char *msg)
{
	racetype       *Race;
	reg int         j;

	Race = races[recpient - 1];
	for (j = 0; j <= MAXGOVERNORS; j++)
		if (Race->governor[j].active)
			push_telegram(recpient, j, msg);
}

/*
 * push_telegram:
 * 
 * arguments: recpient gov msg
 * 
 * called by:
 * 
 * description:  Sends a message to everyone from person to person
 * 
 */
void 
push_telegram(int recpient, int gov, char *msg)
{
	char            telefl[100];
	FILE           *telegram_fd;

	bzero ((char *) telefl, sizeof(telefl));
	sprintf(telefl, "%s.%d.%d", TELEGRAMFL, recpient, gov);

	if ((telegram_fd = fopen(telefl, "a")) == NULL)
		if ((telegram_fd = fopen(telefl, "w+")) == NULL) {
			perror("teleg_send");
			return;
		}
	tm = time(0);
	current_tm = localtime(&tm);

	fprintf(telegram_fd, "%2d/%2d %02d:%02d:%02d %s\n",
	   current_tm->tm_mon + 1, current_tm->tm_mday, current_tm->tm_hour,
		current_tm->tm_min, current_tm->tm_sec, msg);
	fclose(telegram_fd);
}
/*
 * *	read_teleg.c -- (try to) read telegrams *	 the first byte in
 * each telegram is the sending player #, or 254 *	 to denote
 * autoreport.  then the time sent, then the message itself, *	 terminated
 * by TELEG_DELIM.
 */
/*
 * teleg_read:
 * 
 * arguments: Playernum Governor
 * 
 * called by: process_commands
 * 
 * description:  Read the telegrams for the player.  The first byte in each
 * telegram is the sending player number or 254 to denote an autoreport.
 * Then the time send, then the message, then terminated by TELEG_DELIM
 */
void
teleg_read(int Playernum, int Governor)
{
	char           *p;

	bzero((char *) telegram_file, sizeof(telegram_file));
	sprintf(telegram_file, "%s.%d.%d", TELEGRAMFL, Playernum, Governor);

	if ((teleg_read_fd = fopen(telegram_file, "r")) != 0) {
		notify(Playernum, Governor, "Telegrams:");
		stat(telegram_file, &telestat);
		if (telestat.st_size > 0) {
			notify(Playernum, Governor, "\n");
			while (fgets(buf, sizeof buf, teleg_read_fd)) {
				for (p = buf; *p; p++)
					if (*p == '\n') {
						*p = '\0';
						break;
					}
				strcat(buf, "\n");
				notify(Playernum, Governor, buf);
			}
		} else {
			notify(Playernum, Governor, " None.\n");
		}

		fclose(teleg_read_fd);
		teleg_read_fd = fopen(telegram_file, "w+");	/* trunc file */
		fclose(teleg_read_fd);
	} else {
		sprintf(buf, "\nTelegram file %s non-existent.\n", telegram_file);
		notify(Playernum, Governor, buf);
		return;
	}
}
/*
 * news_read:
 * 
 * arguments: Playernum Governor Type
 * 
 * description:  Read the news file
 * 
 */
void 
news_read(int Playernum, int Governor, int type)
{
	char           *p;
	racetype       *Race;

	bzero((char *)telegram_file, sizeof(telegram_file));
	switch (type) {
	case DECLARATION:
		sprintf(telegram_file, "%s", DECLARATIONFL);
		break;
	case TRANSFER:
		sprintf(telegram_file, "%s", TRANSFERFL);
		break;
	case COMBAT:
		sprintf(telegram_file, "%s", COMBATFL);
		break;
	case ANNOUNCE:
		sprintf(telegram_file, "%s", ANNOUNCEFL);
		break;
	default:
		return;
	}

	if ((teleg_read_fd = fopen(telegram_file, "r")) != 0) {
		Race = races[Playernum - 1];
		if (Race->governor[Governor].newspos[type] > newslength[type])
			Race->governor[Governor].newspos[type] = 0;

		fseek(teleg_read_fd, Race->governor[Governor].newspos[type], 0);
		while (fgets(buf, sizeof buf, teleg_read_fd)) {
			for (p = buf; *p; p++)
				if (*p == '\n') {
					*p = '\0';
					break;
				}
			strcat(buf, "\n");
			notify(Playernum, Governor, buf);
		}

		fclose(teleg_read_fd);
		Race->governor[Governor].newspos[type] = newslength[type];
		putrace(Race);
	} else {
		sprintf(buf, "\nNews file %s non-existent.\n", telegram_file);
		notify(Playernum, Governor, buf);
		return;
	}
}
