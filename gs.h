#ifndef GAMESERVER_H
#define GAMESERVER_H

/* - Constants - */
#define VERBOSEPREFIX "-v\0"    // Verbose mode prefix
#define VERBOSEDEFAULT false    // Default verbose mode

#define MAXTRIES 8              // Maximum number of tries
#define MAXTRIESLEN 1           // Maximum number tries len

#define ERR 9
#define NOK 8
#define OK 7
/* ------------- */

/* - Functions -*/
// Helpers
int verifyArg(char **user_args, int num_args, int idx, const char *prefix, void *arg_to_change,
              const void *default_val, bool single_argument);
int verboseMode(int verbose, int PLID, char *request, char *ip, char *port);

// Socket connections 
int TCPConnection(int tcp_fd);
int UDPConnection(int udp_fd, struct sockaddr_in *addr);

// Commands
int commandHandler(char *client_request, char *response);
int startCmd(char *client_request, char *response);
int tryCmd(char *client_request, char *response);
int quitCmd(char *client_request, char *response);
int debugCmd(char *client_request, char *response);

/* ------------- */

#endif