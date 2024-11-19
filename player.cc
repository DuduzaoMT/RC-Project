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

#define MAXPORTSIZE 6           // Maximum port size
#define MAXIPSIZE 40            // Maximum ip size (IPV6 can go up to 39)
#define PORT "58001"            // Default port (58000 + GroupNo)
#define LOCALHOST "127.0.0.1"   // Default host (current computer)

#define GSIPPREFIX "-n\0"       // GSIP's prefix
#define GSPORTPREFIX "-p\0"     // Gsport's prefix

#define USERINPUTBUFFER 128     // Buffer to store user input
#define GENERALSIZEBUFFER 128   // General size to auxiliar buffers

#define MAX_PLAYTIME "600"        // Maximum played time

// Commands
#define STARTCMD "start\0"      
#define TRYCMD "try\0"
#define SHOWTRIALSCMD "show_trials\0" or "st\0"
#define SCOREBOARDCMD "scoreboard\0" or "sb\0"
#define QUITCMD "quit\0"
#define DEBUGCMD "debug\0"

int isNumber(char *s)
{

    for (int i = 0; i < strlen(s); i++)
    {
        if (!isdigit(s[i]))
            return false;
    }
    
    return true;
}

int verifyArg(char **user_args, int idx, const char *prefix, char *arg_to_change, const char *default_val)
{
    if(strcmp(user_args[idx], prefix) == 0)
    {
        if (strcmp(default_val, arg_to_change))
            return 1;

        strcpy(arg_to_change, user_args[idx+1]);
    }
    return 0;
}

int UDPInteraction(char* request,char* response, char* GSIP, char* GSport){

    int fd, errcode, argValid;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;

    // Criação do socket UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    // Configuração do endereço do servidor
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Socket UDP

    errcode = getaddrinfo(GSIP, GSport, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    // Envio de mensagem para o servidor
    n = sendto(fd, request, strlen(request)+1, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("sendto");
        exit(1);
    }

    // Recebimento de resposta do servidor
    addrlen = sizeof(struct sockaddr_in);
    n = recvfrom(fd, response, 128, 0, (struct sockaddr *)res->ai_addr, &addrlen);
    if (n == -1) {
        perror("recvfrom");
        exit(1);
    }

    freeaddrinfo(res);
    close(fd);
    return n;
}

int startCmd(char *arguments,char* GSIP, char* GSport){
    int PLID, max_playtime,parsed_max_playtime,n;
    char PLID_buffer[USERINPUTBUFFER], max_playtime_buffer[USERINPUTBUFFER],final_max_play[16];
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    sscanf(arguments, "%s %s", PLID_buffer, max_playtime_buffer);

    if (strlen(PLID_buffer) == 6 && isNumber(PLID_buffer))
        PLID = atoi(PLID_buffer);
    else{
        fprintf(stderr, "(Invalid PLID)\t");
        return 1;
    }
    parsed_max_playtime = atoi(max_playtime_buffer);
    if (isNumber(max_playtime_buffer) && parsed_max_playtime > 0 && parsed_max_playtime <= 600){
        sprintf(final_max_play,"%03d",parsed_max_playtime);
    }
    else{
        fprintf(stderr, "(Invalid max_playtime)\t");
        return 1;
    }

    sprintf(request,"SNG %d %s\n",PLID,final_max_play);

    // Exibição da resposta do servidor
    if((n = UDPInteraction(request,response, GSIP,GSport) == strlen("RSG OK\n"))){
        fprintf(stdout,"New Game started (max %s sec)\n", final_max_play);
    }
    else{
        fprintf(stderr, "(Connection Lost)\t");
        return 1;
    }

    return 0;
}


int main(int argc, char **argv) {

    char GSIP[MAXIPSIZE] = LOCALHOST;
    char GSport[MAXPORTSIZE] = PORT;

    int player_connected = true;

    // Argument verification
    for (int i = 1; i < argc; i+=2)
    {   
        if (i+1 >= argc){
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }

        if (verifyArg(argv, i, GSIPPREFIX, GSIP, LOCALHOST)){
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }

        if (verifyArg(argv, i, GSPORTPREFIX, GSport, PORT)) {
            fprintf(stderr, "Invalid arguments\n");
            exit(1);  
        }

    }

    printf("IP: %s\nPort: %s\n", GSIP, GSport);
    
    // Player controller
    while (1)
    {
        char command[USERINPUTBUFFER], arguments[USERINPUTBUFFER];
        scanf("%s", command);

        if (!fgets(arguments, sizeof(arguments), stdin)){
            fprintf(stderr, "Invalid command\n");
            continue;
        }
        
        if (strcmp(command, "start") == 0){
            if (startCmd(arguments,GSIP,GSport)){
                fprintf(stderr, "Command error\n");
                continue;
            }
        }
    }

    return 0;
}
