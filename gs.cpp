#include <iostream>
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

using namespace std;

#define MAXPORTSIZE 6         // Maximum port size
#define PORT "58001"          // Default port (58000 + GroupNo)
#define LOCALHOST "127.0.0.1" // Default host (current computer)
#define VERBOSEDEFAULT false  // Default verbose mode

#define VERBOSEPREFIX "-v\0" // Verbose mode prefix
#define GSPORTPREFIX "-p\0"  // Gsport's prefix

#define FALSE 0
#define TRUE 1
#define ERROR 2

int verifyArg(char **user_args, int num_args, int idx, const char *prefix, void *arg_to_change, const void *default_val, bool single_argument)
{
    if(strcmp(user_args[idx], prefix) == 0)
    {
        if (single_argument) { 

            if ((*(int*)default_val) != (*(int*)arg_to_change))
                return ERROR;

            (*(int*) arg_to_change) = true;
            return TRUE;
        }
        else{
            printf("%d, %d\n",(idx)+1 , num_args);
            if (strcmp((char*)default_val, (char*)arg_to_change) || (idx)+1 >= num_args)
                return ERROR;

            strcpy((char*)arg_to_change, user_args[idx+1]);
            return TRUE;
        }

    }
    return FALSE;
}

int main(int argc, char **argv)
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    char GSport[MAXPORTSIZE] = PORT;
    int verbose = VERBOSEDEFAULT, verbose_default = VERBOSEDEFAULT;

    // Argument verification
    int i = 1,verification;
    while (i < argc){

        verification = verifyArg(argv, argc, i, VERBOSEPREFIX, &verbose, &verbose_default, true);

        if (verification == TRUE){
            i++;
            continue;
        }else if (verification == ERROR){
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }

        verification = verifyArg(argv, argc, i, GSPORTPREFIX, GSport, PORT, false);

        if (verification == TRUE){
            i+=2;
            continue;
        }else if (verification == ERROR){
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }
        
        fprintf(stderr, "Invalid arguments\n");
        exit(1);
        
    }

    printf("Port: %s\nVerbose: %d\n", GSport, verbose);

    // Criação do socket UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(1);
    }

    // Configuração do endereço local (localhost)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Socket UDP
    hints.ai_flags = AI_PASSIVE;    // Aceita conexões

    errcode = getaddrinfo(NULL, PORT, &hints, &res);
    if (errcode != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    // Associa o socket ao endereço local
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    {
        perror("bind");
        exit(1);
    }

    // Loop para receber e responder mensagens
    while (1)
    {

        char cmd[3];
        int PLID;

        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        write(1, "Received: ", 10);
        write(1, buffer, n);

        // Envia de volta o mesmo conteúdo recebido
        if (!strncmp(buffer, "SNG", 3))
        {
            n = sendto(fd, "RSG OK\n", 8, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1)
            {
                perror("sendto");
                exit(1);
            }
        }
        else if (!strncmp(buffer, "TRY", 3))
        {
            char C1, C2, C3, C4;
            int trial_number, nW = 1, nB = 2;
            char response[128];

            sscanf(buffer, "TRY %06d %c %c %c %c %d\n", &PLID, &C1, &C2, &C3, &C4, &trial_number);
            sprintf(response, "RTR OK %d %d %d\n", trial_number, nB, nW);
            // sprintf(response,"RTR ENT %c %c %c %c\n",C1,C2,C3,C4);
            // sprintf(response,"RTR ENT %c %c %c %c\n",C1,C2,C3,C4);
            n = sendto(fd, response, 15, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1)
            {
                perror("sendto");
                exit(1);
            }
        }
        else if (!strncmp(buffer, "QUIT", 3))
        {
            n = sendto(fd, "RQT OK\n", 7, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1)
            {
                perror("sendto");
                exit(1);
            }
        }
    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}
