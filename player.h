#ifndef PLAYER_H
#define PLAYER_H

/* - Constants - */
#define MAXIPSIZE 40            // Maximum ip size (IPV6 can go up to 39)
#define LOCALHOST "127.0.0.1"   // Default host (current computer)

#define GSIPPREFIX "-n\0"       // GSIP's prefix
#define GSPORTPREFIX "-p\0"     // Gsport's prefix

#define MAX_PLAYTIME "600"      // Maximum played time

#define CONNECTIONATTEMPS 2     // Number of UDP connection attemps
/* ------------- */


/* - Functions - */
// Helpers
int isNumber(char *s);
int verifyArg(char **user_args, int idx, char *prefix, char *arg_to_change, char *default_val);

// Socket connections
int UDPInteraction(char *request, char *response, char *GSIP, char *GSport);
int TCPInteraction(char *request, char *response, char *GSIP, char *GSport);

// Commands
int startCmd(char *arguments, char *GSIP, char *GSport, int *PLID, int *max_playtime, int *trial_number);
int tryCmd(char *arguments, char *GSIP, char *GSport, int *trial_number, int PLID);
int quitCmd(char *arguments, char *GSIP, char *GSport, int *PLID, int *trial_number);
int exitCmd(char *arguments, char *GSIP, char *GSport, int *PLID, int *trial_number);
int showTrialsCmd(char *GSIP, char *GSport, int PLID);
int scoreBoardCmd(char *GSIP, char *GSport);
int dbgCmd(char *arguments, char *GSIP, char *GSport, int *PLID, int *max_playtime, int *trial_number);
/* ------------- */

#endif