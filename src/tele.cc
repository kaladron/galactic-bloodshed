// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "tele.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "GB_server.h"
#include "buffers.h"
#include "files.h"
#include "files_shl.h"
#include "races.h"
#include "tweakables.h"
#include "vars.h"

static long tm;
static FILE *teleg_read_fd;
static char telegram_file[PATHLEN];
static struct stat telestat;

static struct tm *current_tm; /* for watching for next update */

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
void post(const char *origmsg, int type) {
  const char *telefl;

  switch (type) {
  case DECLARATION:
    telefl = DECLARATIONFL;
    break;
  case TRANSFER:
    telefl = TRANSFERFL;
    break;
  case COMBAT:
    telefl = COMBATFL;
    break;
  case ANNOUNCE:
    telefl = ANNOUNCEFL;
    break;
  default:
    return;
  }

  char *fixmsg = strdupa(origmsg);
  char *p;
  /* look for special symbols */
  for (p = fixmsg; *p; p++) {
    if (*p == ';')
      *p = '\n';
    else if (*p == '|')
      *p = '\t';
  }

  FILE *news_fd;
  if ((news_fd = fopen(telefl, "a")) == NULL) {
    return;
  } else {
    tm = time(0);
    current_tm = localtime(&tm);
    char *outbuf;
    asprintf(&outbuf, "%2d/%2d %02d:%02d:%02d %s", current_tm->tm_mon + 1,
            current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
            current_tm->tm_sec, fixmsg);
    fprintf(news_fd, "%s", outbuf);
    fclose(news_fd);
    newslength[type] += strlen(outbuf);
    free(outbuf);
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
void push_telegram_race(int recpient, char *msg) {
  racetype *Race;
  reg int j;

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
void push_telegram(int recpient, int gov, const char *msg) {
  char telefl[100];
  FILE *telegram_fd;

  bzero((char *)telefl, sizeof(telefl));
  sprintf(telefl, "%s.%d.%d", TELEGRAMFL, recpient, gov);

  if ((telegram_fd = fopen(telefl, "a")) == NULL)
    if ((telegram_fd = fopen(telefl, "w+")) == NULL) {
      perror("tele");
      return;
    }
  tm = time(0);
  current_tm = localtime(&tm);

  fprintf(telegram_fd, "%2d/%2d %02d:%02d:%02d %s\n", current_tm->tm_mon + 1,
          current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
          current_tm->tm_sec, msg);
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
void teleg_read(int Playernum, int Governor) {
  char *p;

  bzero((char *)telegram_file, sizeof(telegram_file));
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
    teleg_read_fd = fopen(telegram_file, "w+"); /* trunc file */
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
void news_read(int Playernum, int Governor, int type) {
  char *p;
  racetype *Race;

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
