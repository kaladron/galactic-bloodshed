/*
** Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
** smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
** Restrictions in GB_copyright.h.
**
**	read_teleg.c -- (try to) read telegrams
**	 the first byte in each telegram is the sending player #, or 254
**	 to denote autoreport.  then the time sent, then the message itself,
**	 terminated by TELEG_DELIM.
*/

#define EXTERN extern
#include "GB_copyright.h"
#include "vars.h"
#include "buffers.h"
#include "races.h"
#include "power.h"
#include "ships.h"
#include <stdio.h> /* for fseek() */
#include <ctype.h>
#include <signal.h>
#include <strings.h>
#include <errno.h>

FILE *fopen(), *teleg_read_fd;

char telegram_file[PATHLEN];

void teleg_read(int, int);
void news_read(int, int, int);
#include "GB_server.p"
#include "files_shl.p"

void teleg_read(int Playernum, int Governor) {
  char *p;
  sprintf(telegram_file, "%s.%d.%d", TELEGRAMFL, Playernum, Governor);

  if ((teleg_read_fd = fopen(telegram_file, "r")) != 0) {
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
    teleg_read_fd = fopen(telegram_file, "w+"); /* trunc file */
    fclose(teleg_read_fd);
  } else {
    sprintf(buf, "\nTelegram file %s non-existent.\n", telegram_file);
    notify(Playernum, Governor, buf);
    return;
  }
}

void news_read(int Playernum, int Governor, int type) {
  char *p;
  racetype *Race;

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
