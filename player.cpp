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
#define TRUE 1
#define FALSE 0
#define ERRO 2

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
        if (rec > 0)
            message_received = 1;

    }
    freeaddrinfo(res);
    close(fd);
    return 0;
}

// Full TCP connection
int TCPInteraction(char* request,char* response, char* GSIP, char* GSport){

    int fd, errcode,nbytes,nleft,nwritten,nread;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    // Create the socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Initialize the hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // Resolve server address
    errcode = getaddrinfo(GSIP, GSport, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    // Connect to the server
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("Connection failed");
        freeaddrinfo(res);
        close(fd);
        exit(1);
    }

    nbytes=strlen(request)+1;
    nleft=nbytes;

    while(nleft>0){
        nwritten=write(fd,request,nleft);
        if(nwritten<=0)/*error*/exit(1);
        nleft-=nwritten;
        request+=nwritten;
    }

    nleft=nbytes;

    while(nleft>0){
        nread=read(fd,response,nleft);
        if(nread==-1)/*error*/exit(1);
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        response+=nread;
    }
    nread=nbytes-nleft;

    // Cleanup
    freeaddrinfo(res);
    close(fd);

    return 0;
}

int startCmd(char *arguments,char* GSIP, char* GSport,int &PLID, int &max_playtime){

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
        return ERRO;
    }
    
    
    if (isNumber(max_playtime_buffer)){
        max_playtime = atoi(max_playtime_buffer);
        if (max_playtime <= 0 || max_playtime > 600){
            fprintf(stderr, "Invalid max_playtime\n");
            return ERRO;
        }
    }

    sprintf(request,"SNG %06d %03d\n",PLID,max_playtime);

    // Exibição da resposta do servidor
    UDPInteraction(request,response, GSIP,GSport);
    if(!strcmp(response,"RSG OK\n")){
        fprintf(stdout,"New Game started (max %d sec)\n", max_playtime);
    }
    else if(!strcmp(response,"RSG NOK\n")){
        fprintf(stdout,"The player with PLID:%06d has an ongoing game", PLID);
        return ERRO;
    }
    else{
        fprintf(stderr, "Connection Lost\n");
        return ERRO;
    }

    // TODO - Inicializar o timer/cronometro
    return FALSE;
}

int tryCmd(char *arguments,char* GSIP, char* GSport, int* trial_number, int PLID){

    char C1,C2,C3,C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    const int n_colors = 6, n_guess = 4;

    int match_colors=0,nB,nW,r_trial_number=-1;

    char colors[n_colors] = {'R','G','B','Y','O','P'};

    int n_args = sscanf(arguments, "%s %s %s %s", &C1,&C2,&C3,&C4);

    if (n_args != 4 || strlen(arguments) != 9){
        fprintf(stderr, "Invalid Colors\n");
        return ERRO;
    }

    char guess_colors[n_guess] = {C1, C2, C3, C4};    

    for (int i = 0; i < n_guess; i++)
        for (int j = 0; j < n_colors; j++)
            if (guess_colors[i] == colors[j])
            {
                match_colors++;
                break;
            }
            
    if (match_colors != 4){
        fprintf(stderr, "Invalid Colors\n");
        return ERRO;
    }

    sprintf(request,"TRY %06d %c %c %c %c %d\n",PLID,C1,C2,C3,C4,(*trial_number));

    // Exibição da resposta do servidor
    UDPInteraction(request,response, GSIP,GSport);
    if(!strncmp(response,"RTR OK",6) ){
        sscanf(response,"RTR OK %d %d %d\n",&r_trial_number,&nB,&nW);
        if (nB == 4){
            fprintf(stdout,"Congrats, you guessed right! SK:%c %c %c %c\n", C1,C2,C3,C4);
            return FALSE;
        }
        fprintf(stdout,"nB = %d, nW=%d\n",nB,nW);
        (*trial_number)++;
    }
    else if(!strncmp(response,"RTR DUP",7)){
        fprintf(stdout,"(Secret Key guess repeats a previous trial`s guess): nB = %d, nW=%d\n",nB,nW);
    }
    //(invalid PLID - not having an ongoing game f.e.)
    else if(!strncmp(response,"RTR NOK",7)){
        fprintf(stderr,"Out of context\n");
        return ERRO;
    }
    // no more attemps available
    else if(!strncmp(response,"RTR ENT",7)){
        sscanf(response,"RTR ENT %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
    }
    // max time exceeded
    else if(!strncmp(response,"RTR ETM",7)){
        sscanf(response,"RTR ETM %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
    }
    else{
        fprintf(stderr, "Connection Lost\n");
        return ERRO;
    }
    return FALSE; 
}

int quitCmd(char* GSIP, char* GSport,int PLID){

    char C1,C2,C3,C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    sprintf(request,"QUIT %06d\n",PLID);

    UDPInteraction(request,response, GSIP,GSport);

    if(!strncmp(response,"RQT OK",6) ){
        sscanf(response,"RQT OK %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
    }
    // PLID not have an ongoing game
    else if(!strncmp(response,"RTR NOK",7)){
        fprintf(stdout,"PLID %06d did not have an ongoing game\n", PLID);
    }
    else{
        fprintf(stderr, "Connection Lost\n");
        return ERRO;
    }
    return FALSE;
}

int exitCmd(char* GSIP, char* GSport,int PLID){
    quitCmd(GSIP,GSport,PLID);
    return TRUE;
}


int main(int argc, char **argv) {

    char GSIP[MAXIPSIZE] = LOCALHOST;
    char GSport[MAXPORTSIZE] = PORT;

    int trial_number = 1;
    int exit_application = FALSE;
    int PLID = 0;
    int max_playtime = 0;
    int r=0;

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

    while (exit_application != TRUE)
    {
        char command[USERINPUTBUFFER], arguments[USERINPUTBUFFER];
        scanf("%s", command);
        
        if (fgets(arguments, sizeof(arguments), stdin)){

            // commands with arguments
            if (!strcmp(command, "start")){
                if (startCmd(arguments,GSIP,GSport,PLID,max_playtime) == ERRO)
                    fprintf(stderr, "Command error\n");
            }
            else if(!strcmp(command,"try")){
                if(tryCmd(arguments,GSIP,GSport,&trial_number,PLID) == ERRO){
                    fprintf(stderr, "Command error\n"); 
                }
            }
            // commands without arguments  
            else if (!strcmp(command, "sb") || !strcmp(command,"scoreboard")){
            }

            else if (!strcmp(command, "st") || !strcmp(command,"show_trials")){
            }

            else if (!strcmp(command, "quit")){
                if(quitCmd(GSIP,GSport,PLID) == ERRO){
                    fprintf(stderr, "Command error\n"); 
                }
            }

            else if (!strcmp(command, "exit")){
                exit_application = exitCmd(GSIP,GSport,PLID);
                if(exit_application == ERRO){
                    fprintf(stderr, "Command error\n"); 
                }
            }
            else{
                fprintf(stderr, "Invalid command\n");
            }
        }
    }

    return 0;
}
