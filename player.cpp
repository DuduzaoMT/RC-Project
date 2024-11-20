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

// Verifies is a set of chars is a number
int isNumber(char *s)
{

    for (int i = 0; i < strlen(s); i++)
    {
        if (!isdigit(s[i]))
            return false;
    }
    
    return true;
}

// Verifies the arguments of the program once it is called
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

// Full UDP connection
int UDPInteraction(char* request,char* response, char* GSIP, char* GSport){

    int fd, errcode, argValid,message_received=0;
    ssize_t send,rec;
    socklen_t addrlen;
    struct addrinfo hints, *res;

    // UDP socket creation
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    // Server info configuration
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // Socket UDP

    errcode = getaddrinfo(GSIP, GSport, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    while (message_received == 0){
        // Send message to server
        send = sendto(fd, request, strlen(request)+1, 0, res->ai_addr, res->ai_addrlen);
        if (send == -1) {
            perror("sendto");
            exit(1);
        }

        // Receive message from server
        addrlen = sizeof(struct sockaddr_in);
        rec = recvfrom(fd, response, 128, 0, (struct sockaddr *)res->ai_addr, &addrlen);

        // TODO - associar um timer para esperar no max 3 segundos pela resposta do servidor

        if (rec == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        if (rec != 0)
            message_received = 1;

    }
    freeaddrinfo(res);
    close(fd);
    return rec;
}

int startCmd(char *arguments,char* GSIP, char* GSport,int &PLID, int &max_playtime){
    int n;
    char PLID_buffer[USERINPUTBUFFER], max_playtime_buffer[USERINPUTBUFFER];
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    // Reset the buffers
    strcpy(PLID_buffer, "");
    strcpy(max_playtime_buffer, "");

    sscanf(arguments, "%s %s", PLID_buffer, max_playtime_buffer);

    if (strlen(PLID_buffer) == 6 && isNumber(PLID_buffer))
        PLID = atoi(PLID_buffer);
        
    else{
        fprintf(stderr, "Invalid PLID\n");
        return 1;
    }
    
    
    if (isNumber(max_playtime_buffer)){
        max_playtime = atoi(max_playtime_buffer);
        if (max_playtime <= 0 || max_playtime > 600){
            fprintf(stderr, "Invalid max_playtime\n");
            return 1;
        }
    }

    sprintf(request,"SNG %06d %03d\n",PLID,max_playtime);

    // Exibição da resposta do servidor
    n = UDPInteraction(request,response, GSIP,GSport);
    if(!strcmp(response,"RSG OK\n")){
        fprintf(stdout,"New Game started (max %d sec)\n", max_playtime);
    }
    else{
        fprintf(stderr, "Connection Lost\n");
        return 1;
    }

    // TODO - Inicializar o timer/cronometro
    return 0;
}

int tryCmd(char *arguments,char* GSIP, char* GSport, int trial_number, int PLID){

    char C1,C2,C3,C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER], expected_response[GENERALSIZEBUFFER];

    int n_colors = 6,n_guess = 4,match_colors=0,n, nB=0,nW=0,r_trial_number=trial_number,try_plid,n_args;
    char colors[n_colors] = "RGBYOP";

    char guess_colors[n_guess];

    n_args = sscanf(arguments, "%06d %c %c %c %c", &try_plid, &C1,&C2,&C3,&C4);

    if( try_plid != PLID){
        fprintf(stderr, "Invalid PLID\n");
        return 1;
    }
    if (n_args != 5){
        fprintf(stderr, "Invalid Colors\n");
        return 1;
    }

    sprintf(guess_colors,"%c%c%c%c",C1,C2,C3,C4);
    for (int i=0;i<n_colors;i++){
        for (int j=0; j<n_guess;j++){
            if(guess_colors[j] == colors[i]){
                match_colors+=1;
            }
        }
    }

    if (match_colors != 4){
        fprintf(stderr, "Invalid Colors\n");
        return 1;
    }

    sprintf(request,"TRY %06d %c %c %c %c %d\n",PLID,C1,C2,C3,C4,trial_number);

    // Exibição da resposta do servidor
    n = UDPInteraction(request,response, GSIP,GSport);
    if(!strncmp(response,"RTR OK",6) ){
        sscanf(response,"RTR OK %d %d %d\n",&r_trial_number,&nB,&nW);

        if (r_trial_number==trial_number){
            fprintf(stdout,"nB = %d, nW=%d\n",nB,nW);
        }
        else{
           fprintf(stderr, "Something went wrong, Try again\n");
           return 1; 
        }
    }
    else{
        fprintf(stderr, "Connection Lost\n");
        return 1;
    }

    return 0;
    
}


int main(int argc, char **argv) {

    char GSIP[MAXIPSIZE] = LOCALHOST;
    char GSport[MAXPORTSIZE] = PORT;

    int trial_number = 1;
    int PLID = 0;
    int max_playtime = 0;
    int end_game = 0;

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

    while (trial_number <= 8 && !end_game)
    {
        char command[USERINPUTBUFFER], arguments[USERINPUTBUFFER];
        scanf("%s", command);
        
        // commands with arguments 
        if (fgets(arguments, sizeof(arguments), stdin)){

            if (!strcmp(command, "start")){
                if (startCmd(arguments,GSIP,GSport,PLID,max_playtime))
                    fprintf(stderr, "Command error\n");
            }
            else if(!strcmp(command,"try")){
                if(tryCmd(arguments,GSIP,GSport,trial_number,PLID))
                    fprintf(stderr, "Command error\n");
                else
                    trial_number++;  
            }
            else
                fprintf(stderr, "Invalid command\n");
        }
        // commands without arguments  
        else{
            if (!strcmp(command, "sb") || !strcmp(command,"scoreboard")){
            }

            else if (!strcmp(command, "st") || !strcmp(command,"show_trials")){
            }

            else if (!strcmp(command, "quit")){
            }

            else if (!strcmp(command, "exit")){
            }
            else{
                fprintf(stderr, "Invalid command\n");
            }
        }
    }

    return 0;
}
