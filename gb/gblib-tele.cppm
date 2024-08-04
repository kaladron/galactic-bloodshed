// SPDX-License-Identifier: Apache-2.0

module;

#include <strings.h>

export module gblib:tele;

import std.compat;
import :types;
import :tweakables;

#include "gb/files.h"

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
export void push_telegram(const player_t recipient, const governor_t gov,
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
  time_t tm = time(nullptr);
  struct tm *current_tm = localtime(&tm);

  fprintf(telegram_fd, "%2d/%2d %02d:%02d:%02d %s\n", current_tm->tm_mon + 1,
          current_tm->tm_mday, current_tm->tm_hour, current_tm->tm_min,
          current_tm->tm_sec, msg.c_str());
  fclose(telegram_fd);
}