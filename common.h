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
#define PORT "58001"            // Default port (58000 + GroupNo)

// Helpers
#define GENERALSIZEBUFFER 2048  // General size to auxiliar buffers

#define UNKNOWN -1
#define FALSE 0
#define TRUE 1
#define ERROR 2
#define RESTART 3

#endif