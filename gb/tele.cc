// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/tele.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

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
void purge() {
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
void post(const std::string fixmsg, int type) {
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

  // look for special symbols
  for (auto p : fixmsg) {
    switch (p) {
      case ';':
        p = '\n';
        break;
      case '|':
        p = '\t';
        break;
    }
  }

  FILE *news_fd;
  if ((news_fd = fopen(telefl, "a")) == nullptr) {
    return;
  }
  tm = time(nullptr);
  current_tm = localtime(&tm);
  char *outbuf;
  if (asprintf(&outbuf, "%2d/%2d %02d:%02d:%02d %s", current_tm->tm_mon + 1,
               current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
               current_tm->tm_sec, fixmsg.c_str()) < 0) {
    perror("Gaaaaah");
    exit(-1);
  }
  fprintf(news_fd, "%s", outbuf);
  fclose(news_fd);
  newslength[type] += strlen(outbuf);
  free(outbuf);
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
void push_telegram_race(const player_t recipient, const std::string &msg) {
  int j;

  auto &race = races[recipient - 1];
  for (j = 0; j <= MAXGOVERNORS; j++)
    if (race.governor[j].active) push_telegram(recipient, j, msg);
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
void push_telegram(const player_t recipient, const governor_t gov,
                   const std::string &msg) {
  char telefl[100];
  FILE *telegram_fd;

  bzero((char *)telefl, sizeof(telefl));
  sprintf(telefl, "%s.%d.%d", TELEGRAMFL, recipient, gov);

  if ((telegram_fd = fopen(telefl, "a")) == nullptr)
    if ((telegram_fd = fopen(telefl, "w+")) == nullptr) {
      perror("tele");
      return;
    }
  tm = time(nullptr);
  current_tm = localtime(&tm);

  fprintf(telegram_fd, "%2d/%2d %02d:%02d:%02d %s\n", current_tm->tm_mon + 1,
          current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
          current_tm->tm_sec, msg.c_str());
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
void teleg_read(GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  char *p;

  bzero((char *)telegram_file, sizeof(telegram_file));
  sprintf(telegram_file, "%s.%d.%d", TELEGRAMFL, Playernum, Governor);

  if ((teleg_read_fd = fopen(telegram_file, "r")) != nullptr) {
    g.out << "Telegrams:";
    stat(telegram_file, &telestat);
    if (telestat.st_size > 0) {
      g.out << "\n";
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
      g.out << " None.\n";
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

  if ((teleg_read_fd = fopen(telegram_file, "r")) != nullptr) {
    auto &race = races[Playernum - 1];
    if (race.governor[Governor].newspos[type] > newslength[type])
      race.governor[Governor].newspos[type] = 0;

    fseek(teleg_read_fd, race.governor[Governor].newspos[type] & LONG_MAX,
          SEEK_SET);
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
    race.governor[Governor].newspos[type] = newslength[type];
    putrace(race);
  } else {
    sprintf(buf, "\nNews file %s non-existent.\n", telegram_file);
    notify(Playernum, Governor, buf);
    return;
  }
}

/**
 * \brief Check for telegrams and notify the player if there is any.
 * \arg g Game object
 */
void check_for_telegrams(GameObj &g) {
  std::string filename = std::string(TELEGRAMFL) + '.' +
                         std::to_string(g.player) + '.' +
                         std::to_string(g.governor);
  struct stat sbuf;
  std::memset(&sbuf, 0, sizeof(sbuf));
  stat(filename.c_str(), &sbuf);
  if (sbuf.st_size != 0) {
    g.out << "You have telegram(s) waiting. Use 'read' to read them.\n";
  }
}
