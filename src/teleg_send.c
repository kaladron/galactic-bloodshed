/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 *	teleg_send.c -- does the work of sending messages
 */

#define EXTERN extern
#include "GB_copyright.h"
#include "vars.h"
#include "files.h"
#include "races.h"
#include "buffers.h"
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/time.h>

long tm;
char *ctime();

void purge(void);
void post(char *, int);
void push_telegram_race(int, char *);
void push_telegram(int, int, char *);

struct tm *current_tm; /* for watching for next update */

/* trims the new files */
void purge(void) {
  fclose(fopen(DECLARATIONFL, "w+"));
  newslength[0] = 0;
  fclose(fopen(COMBATFL, "w+"));
  newslength[1] = 0;
  fclose(fopen(ANNOUNCEFL, "w+"));
  newslength[2] = 0;
  fclose(fopen(TRANSFERFL, "w+"));
  newslength[3] = 0;
}

void post(char *msg, int type) {
  char telefl[100];
  char pbuf[1024]; /* this is needed, don't use global pointer! */
  FILE *news_fd;
  char *p;

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
    sprintf(pbuf, "%2d/%2d %02d:%02d:%02d %s", current_tm->tm_mon + 1,
            current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
            current_tm->tm_sec, msg);
    fprintf(news_fd, "%s", pbuf);
    fclose(news_fd);
    newslength[type] += strlen(pbuf);
  }
}

void push_telegram_race(int recpient, char *msg) {
  racetype *Race;
  reg int j;

  Race = races[recpient - 1];
  for (j = 0; j <= MAXGOVERNORS; j++)
    if (Race->governor[j].active)
      push_telegram(recpient, j, msg);
}

void push_telegram(int recpient, int gov, char *msg) {
  char telefl[100];
  FILE *telegram_fd;

  sprintf(telefl, "%s.%d.%d", TELEGRAMFL, recpient, gov);

  if ((telegram_fd = fopen(telefl, "a")) == NULL)
    if ((telegram_fd = fopen(telefl, "w+")) == NULL) {
      perror("teleg_send");
      return;
    }

  tm = time(0);
  current_tm = localtime(&tm);
  fprintf(telegram_fd, "%2d/%2d %02d:%02d:%02d %s\n", current_tm->tm_mon + 1,
          current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
          current_tm->tm_sec, msg);
  fclose(telegram_fd);
}
