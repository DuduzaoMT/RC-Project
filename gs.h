#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <sys/stat.h>
#include <time.h>
#include <filesystem>
#include <dirent.h>

/* - Constants - */
#define VERBOSEPREFIX "-v\0"    // Verbose mode prefix
#define VERBOSEDEFAULT false    // Default verbose mode

#define MAXTRIES 8              // Maximum number of tries
#define MAXTRIESLEN 1           // Maximum number tries len

#define ERR 9
#define NOK 8
#define OK 7
/* ------------- */

/* - Macros - */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/* ---------- */

typedef struct scorelist
{
    int score[10];
    char PLID[10][7];
    char colors[10][5];
    int num_tries[10];
    char mode[10][6];
    int nscores;
} Scorelist;


/* - Functions -*/
// Helpers
int verifyArg(char **user_args, int num_args, int idx, const char *prefix, void *arg_to_change,
              const void *default_val, bool single_argument);
int verboseMode(int verbose, int PLID, char *request, char *ip, char *port);

// - File reading/writing
int gameAlreadyEnded(char *file_name);
int storeResult(char *file_name, char code);
int getDupGuessAndTrialNumber(FILE *player_fd, char *guess_colours, bool *dup, int *trial_number);
int startGame(char *PLID, char *time_buffer, char *colors, char mode, char *opcode);
int readTrials(char *PLID, char *response);
int findLastGame(char *PLID, char *response);
int addScore(char *PLID, char *response);
int findTopScores(Scorelist *list);

// - General
void getColours(char *colours);
int getBlackAndWhite(int *nB, int *nW, char *colours, char *guess_colours);

// Socket connections 
int TCPConnection(int tcp_fd, int verbose);
int UDPConnection(int udp_fd, struct sockaddr_in *addr, int verbose);

// Commands
int commandHandler(char *client_request, char *response);
int startCmd(char *client_request, char *response);
int tryCmd(char *client_request, char *response);
int quitCmd(char *client_request, char *response);
int debugCmd(char *client_request, char *response);
int showTrialsCmd(char * client_request, char * response);
int scoreboardCmd(char *client_request, char *response);
/* ------------- */

#endif