#ifndef GAMESERVER_H
#define GAMESERVER_H

/* - Constants - */
#define VERBOSEPREFIX "-v\0"    // Verbose mode prefix
#define VERBOSEDEFAULT false    // Default verbose mode
/* ------------- */

/* - Functions -*/
// Helpers
int verifyArg(char **user_args, int num_args, int idx, const char *prefix, void *arg_to_change,
              const void *default_val, bool single_argument);
/* ------------- */

#endif