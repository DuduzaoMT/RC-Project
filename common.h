#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

// Commands
#define GSPORTPREFIX "-p\0"     // Gsport's prefix
#define MAXPORTSIZE 6           // Maximum port size
#define PORT "58057"            // Default port (58000 + GroupNo)

// Helpers
#define GENERALSIZEBUFFER 2048  // General size to auxiliar buffers
#define USERINPUTBUFFER 128     // Buffer to store user input

#define PLIDSIZE 7              // Player ID
#define COLORBUFFERSIZE 4       // Number of colors

#define UNKNOWN -1
#define FALSE 0
#define TRUE 1
#define ERROR 2
#define RESTART 3

// Function Helpers
int isNumber(char *s);
int verifyStartCmd(char *PLID_buffer, char *max_playtime_buffer);
int verifyTryCmd(char C1, char C2, char C3, char C4);

#endif