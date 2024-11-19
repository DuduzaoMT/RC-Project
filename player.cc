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

#define USERINPUTBUFFER 32      // Buffer to store user input

// Commands
#define STARTCMD "start\0"      
#define TRYCMD "try\0"
#define SHOWTRIALSCMD "show_trials\0" or "st\0"
#define SCOREBOARDCMD "scoreboard\0" or "sb\0"
#define QUITCMD "quit\0"
#define DEBUGCMD "debug\0"

void parseInput(vector<vector<char>> command){

    char argument[USERINPUTBUFFER];

    while (scanf("%s", argument)){
        vector<char> parsed(argument, argument+USERINPUTBUFFER);
        command.push_back(parsed);
        
    }
    printf("ola\n");
}

int main(int argc, char **argv) {

    int fd, errcode, argValid;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    char buffer[128];

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

        if(strcmp(argv[i], GSIPPREFIX) == 0)
            strcpy(GSIP, argv[i+1]);
        
        if(strcmp(argv[i], GSPORTPREFIX) == 0)
            strcpy(GSport, argv[i+1]);
    }

    printf("IP: %s\nPort: %s\n", GSIP, GSport);
    
    // Player controller
    vector<vector<char>> command;
    parseInput(command);
    printf("%lu\n", command.size());
    for (int i = 0; i < command.size(); i++)
    {
        printf("%s\n", command[i].data());
    }
    

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
    n = sendto(fd, "Hello!\n", 7, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("sendto");
        exit(1);
    }

    // Recebimento de resposta do servidor
    addrlen = sizeof(struct sockaddr_in);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)res->ai_addr, &addrlen);
    if (n == -1) {
        perror("recvfrom");
        exit(1);
    }

    // Exibição da resposta do servidor
    write(1, "Echo: ", 6);
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);

    return 0;
}
