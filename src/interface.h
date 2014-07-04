#include "TinyMUD_copyright.h"

/* these symbols must be defined by the interface */
extern int notify();
extern int shutdown_flag; /* if non-zero, interface should shut down */
extern void emergency_shutdown();

/* the following symbols are provided by game.c */

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 512
#define BUFFER_LEN ((MAX_COMMAND_LEN)*8)
extern void process_command();

extern void panic();
