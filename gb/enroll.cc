// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// enroll - racegen interface for Galactic Bloodshed race enrollment program.

#include "gb/enroll.h"

import std;
import gblib;

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gb/racegen.h"

static const char* DEFAULT_ENROLLMENT_FILENAME = "enroll.saves";
static const char* DEFAULT_ENROLLMENT_FAILURE_FILENAME = "failures.saves";

/*
 * Returns: 0 if the race was successfully enrolled, or 1 if not.
 */
static int enroll_player_race(const char* failure_filename) {
  char c[128];
  FILE* g;
  int n;
  static int recursing = 0;
  static int successful_enroll_in_fix_mode = 0;

  while ((n = critique_to_file(nullptr, 1, 1))) {
    std::println("Race ({}) unacceptable, for the following reason{}:",
                 race_info.name, (n > 1) ? 's' : '\0');
    critique_to_file(stdout, 1, 1);
    if (recursing) {
      std::println("\"Quit\" to break out of fix mode.");
      std::println("Enroll failed.");
      return 1;
    }
    if (race_info.status == STATUS_ENROLLED) return 0;
    n = Dialogue("Abort, enroll anyway, fix, mail rejection?", "abort",
                 "enroll", "fix", "mail", 0);
    if (n == 1) /* enroll anyway */
      break;
    if (n == 2) { /* fix */
      std::println(R"(Recursive racegen.  "Enroll" or "Quit" to exit.)");
      recursing = 1;
      modify_print_loop(1);
      please_quit = recursing = 0;
      if (successful_enroll_in_fix_mode) {
        successful_enroll_in_fix_mode = 0;
        return 0;
      }
      continue;
    }
    if (failure_filename != nullptr) {
      if (nullptr == fopen(failure_filename, "w+")) {
        std::println("Warning: unable to open failures file \"{}\".",
                     failure_filename);
        std::println("Race not saved to failures file.");
      } else {
        // print_to_file(f, 0) ; // TODO(jeffbailey): What was this supposed to
        // do?
        std::println("Race appended to failures file \"{}\".",
                     failure_filename);
        // fclose(f) ;
      }
    }
    if (n == 0) /* Abort */
      return 1;

    g = fopen(TMP, "w");
    if (g == nullptr) {
      std::println("Unable to open file \"{}\".", TMP);
      return 1;
    }
    std::println(g, "To: {}", race_info.address);
    std::println(g, "Subject: {} Race Rejection", GAME);
    std::println(g, "");
    std::println(g,
                 "The race you submitted ({}) was not accepted, for the "
                 "following reason{}:",
                 race_info.name, (n > 1) ? 's' : '\0');
    critique_to_file(g, 1, 1);
    std::println(g, "");
    std::println(g, "Please re-submit a race if you want to play in {}.", GAME);
    std::println(g, "(Check to make sure you are using racegen {})", VERSION);
    std::println(g, "");
    std::println(g, "For verification, here is my understanding of your race:");
    print_to_file(g, 1);
    fclose(g);

    std::print("Sending critique to {} via {}...", race_info.address, MAILER);
    fflush(stdout);
    sprintf(c, "cat %s | %s %s", TMP, MAILER, race_info.address);
    if (system(c) < 0) {
      perror("gaaaaaah");
      exit(-1);
    }
    std::println("done.");

    return 1;
  }

  if (enroll_valid_race()) return enroll_player_race(failure_filename);

  if (recursing) {
    successful_enroll_in_fix_mode = 1;
    please_quit = 1;
  }

  g = fopen(TMP, "w");
  if (g == nullptr) {
    std::println("Unable to open file \"{}\".", TMP);
    return 0;
  }
  std::println(g, "To: {}", race_info.address);
  std::println(g, "Subject: {} Race Accepted", GAME);
  std::println(g, "");
  std::println(g, "The race you submitted ({}) was accepted.", race_info.name);
#if 0
  if (race.modified_by_diety) {
    fprintf(g, "The race was altered in order to be acceptable.\n") ;
    fprintf(g, "Your race now looks like this:\n") ;
    fprintf(g, "\n") ;
    print_to_file(g, verbose, 0) ;
    fprintf(g, "\n") ;
    }
#endif
  fclose(g);

  std::print("Sending acceptance to {} via {}...", race_info.address, MAILER);
  fflush(stdout);
  sprintf(c, "cat %s | %s %s", TMP, MAILER, race_info.address);
  if (system(c) < 0) {
    perror("gaaaaaah");
    exit(-1);
  }
  std::println("done.");

  return 0;
}

int enroll(int argc, const char* argv[]) {
  int ret;
  FILE* g;

  if (argc < 2) argv[1] = DEFAULT_ENROLLMENT_FAILURE_FILENAME;
  g = fopen(argv[1], "w+");
  if (g == nullptr)
    std::println("Unable to open failures file \"{}\".", argv[1]);
  fclose(g);
  bcopy(&race_info, &last, sizeof(struct x));

  /*
   * race.address will be unequal to TO in the instance that this is a
   * race submission mailed from somebody other than the moderator.  */
  if (strcmp(race_info.address, TO) != 0)
    ret = enroll_player_race(argv[1]);
  else if ((ret = critique_to_file(nullptr, 1, 0))) {
    std::println("Race ({}) unacceptable, for the following reason{}:",
                 race_info.name, (ret > 1) ? 's' : '\0');
    critique_to_file(stdout, 1, 0);
  } else if ((ret = enroll_valid_race()))
    critique_to_file(stdout, 1, 0);

  if (ret) std::println("Enroll failed.");
  return ret;
}

/**************
 * Iteratively loads races from a file, and enrolls them.
 */
void process(int argc, const char* argv[]) {
  FILE* f;
  FILE* g;
  int n;
  int nenrolled;

  if (argc < 2) argv[1] = DEFAULT_ENROLLMENT_FILENAME;
  f = fopen(argv[1], "r");
  if (f == nullptr) {
    std::println("Unable to open races file \"{}\".", argv[1]);
    return;
  }

  if (argc < 3) argv[2] = DEFAULT_ENROLLMENT_FAILURE_FILENAME;
  g = fopen(argv[2], "w");
  if (g == nullptr)
    std::println("Unable to open failures file \"{}\".", argv[2]);
  fclose(g);

  n = 0;
  nenrolled = 0;
  while (!feof(f)) {
    if (!load_from_file(f)) continue;
    n++;
    std::println("{}, from {}", race_info.name, race_info.address);
    /* We need the side effects: */
    last_npoints = npoints;
    npoints = STARTING_POINTS - cost_of_race();
    if (!enroll_player_race(argv[2])) nenrolled += 1;
  }
  fclose(f);

  std::println("Enrolled {} race{}; {} failure{} saved in file {}.", nenrolled,
               (nenrolled != 1) ? 's' : '\0', n - nenrolled,
               (n - nenrolled != 1) ? 's' : '\0', argv[2]);
}
