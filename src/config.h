#define VERS "v5.2 10/9/92" /* don't change this */

#define GB_HOST "solana.mps.ohio-state.edu" /* change this for your machine */
#define GB_PORT 2010 /* change this for your port selection */

#define COMMAND_TIME_MSEC 250 /* time slice length in milliseconds */
#define COMMANDS_PER_TIME            \
  1 /* commands per time slice after \
       burst */
#define COMMAND_BURST_SIZE                                \
  250                        /* commands allowed per user \
                                in a burst */
#define DISCONNECT_TIME 7200 /* maximum idle time */
#define MAX_OUTPUT 32768     /* don't change this */

/* don't change these */
#define QUIT_COMMAND "quit"
#define WHO_COMMAND "who"
#define HELP_COMMAND "help"
#define EMULATE_COMMAND                                 \
  "emulate" /* allows deity to emulate another player - \
              invaluable for debugging */

#define WELCOME_FILE "welcome.txt"
#define HELP_FILE DOCDIR "help.txt"
#define LEAVE_MESSAGE "\n*** Thank you for playing Galactic Bloodshed ***\n"

#undef EXTERNAL_TRIGGER /* if you wish to allow the below passwords to \
                             trigger updates and movement segments */
#ifdef EXTERNAL_TRIGGER
#define UPDATE_PASSWORD "put_your_update_password_here"
#define SEGMENT_PASSWORD "put_your_segment_password_here"
#endif

#define MARKET      /* comment this out if you don't want to use the market */
#undef VICTORY      /* if you want to use victory conditions */
#undef DISSOLVE     /* If you want to allow players to dissolve */
#define DEFENSE     /* If you want to allow planetary guns */
#define VOTING      /* If you want to allow player voting. */
#undef ACCESS_CHECK /* If you want to check address authorization. */
#undef NOMADS       /* If you want to allow min # sexes to always colonize \
a sector safely */
#define MONITOR /* allows deity to monitor messages etc (deity can set with \
                   'toggle monitor' option. I use it to watch battles in    \
                   in progress. -G */
#undef SHOW_COWARDS  /* If you want the number of invisible players to be \
   shown to other players */
#undef POD_TERRAFORM /* If pods will terraform sectors they infect */
