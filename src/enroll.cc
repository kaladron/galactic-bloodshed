// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

// enroll - racegen interface for Galactic Bloodshed race enrollment program.

#include "enroll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_info.h"
#include "racegen.h"

#define DEFAULT_ENROLLMENT_FILENAME "enroll.saves"
#define DEFAULT_ENROLLMENT_FAILURE_FILENAME "failures.saves"

extern int enroll_valid_race();

static int enroll_player_race(char *failure_filename);

/*
 * Returns: 0 if the race was successfully enrolled, or 1 if not.
 */
static int enroll_player_race(char *failure_filename) {
  char c[128];
  FILE *g;
  int n;
  static int recursing = 0;
  static int successful_enroll_in_fix_mode = 0;

  while ((n = critique_to_file(NULL, 1, 1))) {
    printf("Race (%s) unacceptable, for the following reason%c:\n",
           race_info.name, (n > 1) ? 's' : '\0');
    critique_to_file(stdout, 1, 1);
    if (recursing) {
      printf("\"Quit\" to break out of fix mode.\n");
      return 1;
    }
    if (race_info.status == STATUS_ENROLLED) return 0;
    n = Dialogue("Abort, enroll anyway, fix, mail rejection?", "abort",
                 "enroll", "fix", "mail", 0);
    if (n == 1) /* enroll anyway */
      break;
    if (n == 2) { /* fix */
      printf("Recursive racegen.  \"Enroll\" or \"Quit\" to exit.\n");
      recursing = 1;
      modify_print_loop(1);
      please_quit = recursing = 0;
      if (successful_enroll_in_fix_mode) {
        successful_enroll_in_fix_mode = 0;
        return 0;
      }
      continue;
    }
    if (failure_filename != NULL) {
      if (NULL == fopen(failure_filename, "w+")) {
        printf("Warning: unable to open failures file \"%s\".\n",
               failure_filename);
        printf("Race not saved to failures file.\n");
      } else {
        // print_to_file(f, 0) ; // TODO(jeffbailey): What was this supposed to
        // do?
        printf("Race appended to failures file \"%s\".\n", failure_filename);
        // fclose(f) ;
      }
    }
    if (n == 0) /* Abort */
      return 1;

    g = fopen(TMP, "w");
    if (g == NULL) {
      printf("Unable to open file \"%s\".\n", TMP);
      return 1;
    }
    fprintf(g, "To: %s\n", race_info.address);
    fprintf(g, "Subject: %s Race Rejection\n", GAME);
    fprintf(g, "\n");
    fprintf(g,
            "The race you submitted (%s) was not accepted, for the "
            "following reason%c:\n",
            race_info.name, (n > 1) ? 's' : '\0');
    critique_to_file(g, 1, 1);
    fprintf(g, "\n");
    fprintf(g, "Please re-submit a race if you want to play in %s.\n", GAME);
    fprintf(g, "(Check to make sure you are using racegen %s)\n", VERSION);
    fprintf(g, "\n");
    fprintf(g, "For verification, here is my understanding of your race:\n");
    print_to_file(g, 1);
    fclose(g);

    printf("Sending critique to %s via %s...", race_info.address, MAILER);
    fflush(stdout);
    sprintf(c, "cat %s | %s %s", TMP, MAILER, race_info.address);
    system(c);
    printf("done.\n");

    return 1;
  }

  if (enroll_valid_race()) return enroll_player_race(failure_filename);

  if (recursing) {
    successful_enroll_in_fix_mode = 1;
    please_quit = 1;
  }

  g = fopen(TMP, "w");
  if (g == NULL) {
    printf("Unable to open file \"%s\".\n", TMP);
    return 0;
  }
  fprintf(g, "To: %s\n", race_info.address);
  fprintf(g, "Subject: %s Race Accepted\n", GAME);
  fprintf(g, "\n");
  fprintf(g, "The race you submitted (%s) was accepted.\n", race_info.name);
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

  printf("Sending acceptance to %s via %s...", race_info.address, MAILER);
  fflush(stdout);
  sprintf(c, "cat %s | %s %s", TMP, MAILER, race_info.address);
  system(c);
  printf("done.\n");

  return 0;
}

int enroll(int argc, char *argv[]) {
  int ret;
  FILE *g;

  if (argc < 2) argv[1] = DEFAULT_ENROLLMENT_FAILURE_FILENAME;
  g = fopen(argv[1], "w+");
  if (g == NULL) printf("Unable to open failures file \"%s\".\n", argv[1]);
  fclose(g);
  bcopy(&race_info, &last, sizeof(struct x));

  /*
   * race.address will be unequal to TO in the instance that this is a
   * race submission mailed from somebody other than the moderator.  */
  if (strcmp(race_info.address, TO))
    ret = enroll_player_race(argv[1]);
  else if ((ret = critique_to_file(NULL, 1, 0))) {
    printf("Race (%s) unacceptable, for the following reason%c:\n",
           race_info.name, (ret > 1) ? 's' : '\0');
    critique_to_file(stdout, 1, 0);
  } else if ((ret = enroll_valid_race()))
    critique_to_file(stdout, 1, 0);

  if (ret) printf("Enroll failed.\n");
  return ret;
}

/**************
 * Iteratively loads races from a file, and enrolls them.
 */
void process(int argc, char *argv[]) {
  FILE *f, *g;
  int n, nenrolled;

  if (argc < 2) argv[1] = DEFAULT_ENROLLMENT_FILENAME;
  f = fopen(argv[1], "r");
  if (f == NULL) {
    printf("Unable to open races file \"%s\".\n", argv[1]);
    return;
  }

  if (argc < 3) argv[2] = DEFAULT_ENROLLMENT_FAILURE_FILENAME;
  g = fopen(argv[2], "w");
  if (g == NULL) printf("Unable to open failures file \"%s\".\n", argv[2]);
  fclose(g);

  n = 0;
  nenrolled = 0;
  while (!feof(f)) {
    if (!load_from_file(f)) continue;
    n++;
    printf("%s, from %s\n", race_info.name, race_info.address);
    /* We need the side effects: */
    last_npoints = npoints;
    npoints = STARTING_POINTS - cost_of_race();
    if (!enroll_player_race(argv[2])) nenrolled += 1;
  }
  fclose(f);

  printf("Enrolled %d race%c; %d failure%c saved in file %s.\n", nenrolled,
         (nenrolled != 1) ? 's' : '\0', n - nenrolled,
         (n - nenrolled != 1) ? 's' : '\0', argv[2]);
}
