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

// Helpers
#define UNKNOWN -1
#define FALSE 0
#define TRUE 1
#define ERROR 2
#define RESTART 3

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
        send = sendto(fd, request, strlen(request), 0, res->ai_addr, res->ai_addrlen);
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

    nbytes=strlen(request);
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

int startCmd(char *arguments,char* GSIP, char* GSport,int *PLID, int *max_playtime,int *trial_number){

    int new_PLID, new_max_playtime;

    char PLID_buffer[USERINPUTBUFFER], max_playtime_buffer[USERINPUTBUFFER];
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    // Reset the buffers
    strcpy(PLID_buffer, "");
    strcpy(max_playtime_buffer, "");

    sscanf(arguments, "%s %s", PLID_buffer, max_playtime_buffer);

    if (strlen(PLID_buffer) == 6 && isNumber(PLID_buffer))
        new_PLID = atoi(PLID_buffer);
    else{
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }
    
    
    if (isNumber(max_playtime_buffer)){
        new_max_playtime = atoi(max_playtime_buffer);
        if (new_max_playtime <= 0 || new_max_playtime > 600){
            fprintf(stderr, "Invalid max_playtime\n");
            return ERROR;
        }
    }

    sprintf(request,"SNG %06d %03d\n",new_PLID,new_max_playtime);

    // Exibição da resposta do servidor
    UDPInteraction(request,response, GSIP,GSport);
    if(!strcmp(response,"RSG OK\n")){
        fprintf(stdout,"New Game started (max %d sec)\n", new_max_playtime);
    }
    else if(!strcmp(response,"RSG NOK\n")){
        fprintf(stdout,"The player with PLID:%06d has an ongoing game", (*PLID));
        return ERROR;
    }
    else
        return ERROR;
    
    (*trial_number) = 1;
    (*PLID) = new_PLID;
    (*max_playtime) = new_max_playtime;


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
        return ERROR;
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
        return ERROR;
    }

    sprintf(request,"TRY %06d %c %c %c %c %d\n",PLID,C1,C2,C3,C4,(*trial_number));

    // Exibição da resposta do servidor
    UDPInteraction(request,response, GSIP,GSport);
    
    if(!strncmp(response,"RTR OK",6) ){
        sscanf(response,"RTR OK %d %d %d\n",&r_trial_number,&nB,&nW);
        if (nB == 4){
            fprintf(stdout,"Congrats, you guessed right! SK:%c %c %c %c\n", C1,C2,C3,C4);
            return RESTART;
        }
        fprintf(stdout,"nB = %d, nW=%d\n",nB,nW);
        (*trial_number)++;
    }
    else if(!strncmp(response,"RTR DUP",7)){
        fprintf(stdout,"Secret Key guess repeats a previous trial`s guess: nB = %d, nW=%d\n",nB,nW);
    }
    //(invalid PLID - not having an ongoing game f.e.)
    else if(!strncmp(response,"RTR NOK",7) || PLID == UNKNOWN){
        fprintf(stderr,"Out of context (probabily there is not an ongoing game)\n");
        return ERROR;
    }
    // no more attemps available
    else if(!strncmp(response,"RTR ENT",7)){
        sscanf(response,"RTR ENT %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
        return RESTART;
    }
    // max time exceeded
    else if(!strncmp(response,"RTR ETM",7)){
        sscanf(response,"RTR ETM %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
        return RESTART;
    }
    else
        return ERROR;
    
    return FALSE; 
}

int quitCmd(char* GSIP, char* GSport,int PLID){

    char C1,C2,C3,C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    sprintf(request,"QUT %06d\n",PLID);

    UDPInteraction(request,response, GSIP,GSport);

    if(!strncmp(response,"RQT OK",6) ){
        sscanf(response,"RQT OK %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
        return RESTART;
    }
    // PLID not have an ongoing game
    else if(!strncmp(response,"RQT NOK",7)){
        fprintf(stdout,"There`s not an ongoing game\n");
    }
    else
        return ERROR;
    
    return FALSE;
}

int exitCmd(char* GSIP, char* GSport,int PLID){
    char C1,C2,C3,C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    sprintf(request,"QUT %06d\n",PLID);

    UDPInteraction(request,response, GSIP,GSport);

    if(!strncmp(response,"RQT OK",6) ){
        sscanf(response,"RQT OK %c %c %c %c\n",&C1,&C2,&C3,&C4);
        fprintf(stdout,"Secret Key was: %c %c %c %c\n", C1,C2,C3,C4);
    }
    else if (!strncmp(response,"RQT ERR",6))
        return ERROR;
    
    return TRUE;
}

int showTrialsCmd(char* GSIP, char* GSport,int PLID){

    char request[GENERALSIZEBUFFER], response[80000];

    sprintf(request,"STR %06d\n",PLID);

    TCPInteraction(request,response, GSIP,GSport);

    printf("%s\n", response);

    return 0;

}

int debugCmd(char *arguments,char* GSIP, char* GSport,int *PLID, int *max_playtime,int *trial_number){

    int new_PLID, new_max_playtime;

    char PLID_buffer[USERINPUTBUFFER], max_playtime_buffer[USERINPUTBUFFER], C1, C2, C3, C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    // Reset the buffers
    strcpy(PLID_buffer, "");
    strcpy(max_playtime_buffer, "");

    sscanf(arguments, "%s %s %c %c %c %c", PLID_buffer, max_playtime_buffer, &C1, &C2, &C3, &C4);

    if (strlen(PLID_buffer) == 6 && isNumber(PLID_buffer))
        new_PLID = atoi(PLID_buffer);
    else{
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }
    
    
    if (isNumber(max_playtime_buffer)){
        new_max_playtime = atoi(max_playtime_buffer);
        if (new_max_playtime <= 0 || new_max_playtime > 600){
            fprintf(stderr, "Invalid max_playtime\n");
            return ERROR;
        }
    }

    sprintf(request,"DBG %06d %03d %c %c %c %c\n",new_PLID,new_max_playtime, C1, C2, C3, C4);

    // Exibição da resposta do servidor
    UDPInteraction(request,response, GSIP,GSport);

    if(!strcmp(response,"RDB OK\n")){
        fprintf(stdout,"New Game started (max %d sec)\n", new_max_playtime);
    }
    else if(!strcmp(response,"RDB NOK\n")){
        fprintf(stdout,"The player with PLID:%06d has an ongoing game", (*PLID));
        return ERROR;
    }
    else
        return ERROR;
    
    (*trial_number) = 1;
    (*PLID) = new_PLID;
    (*max_playtime) = new_max_playtime;


    // TODO - Inicializar o timer/cronometro
    return FALSE;
}

int main(int argc, char **argv) {

    char GSIP[MAXIPSIZE] = LOCALHOST;
    char GSport[MAXPORTSIZE] = PORT;

    int trial_number = 1;
    int exit_application = FALSE;
    int PLID = UNKNOWN;
    int max_playtime = UNKNOWN;

    int command_status = -1;

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
        printf("-----\nPLID atual: %d\ntrial_number atual: %d\nmax_playtime atual: %d\n-----\n", PLID, trial_number, max_playtime);

        char command[USERINPUTBUFFER], arguments[USERINPUTBUFFER];
        scanf("%s", command);
        
        if (fgets(arguments, sizeof(arguments), stdin)){

            // commands with arguments
            if (!strcmp(command, "start")){
                if(startCmd(arguments,GSIP,GSport,&PLID,&max_playtime,&trial_number) == ERROR){
                    fprintf(stderr, "Start command error\n"); 
                }
            }
            else if(!strcmp(command,"try")){
                command_status = tryCmd(arguments,GSIP,GSport,&trial_number,PLID);
                if (command_status == ERROR)
                    fprintf(stderr, "Try command error\n");
                else if (command_status == RESTART){
                    PLID = UNKNOWN;
                    trial_number = 1;
                    max_playtime = UNKNOWN;
                }
            }
            else if (!strcmp(command, "debug")){
                if(debugCmd(arguments,GSIP,GSport,&PLID,&max_playtime,&trial_number) == ERROR){
                    fprintf(stderr, "Debug command error\n"); 
                }
            }
            // commands without arguments  
            else if (!strcmp(command, "sb") || !strcmp(command,"scoreboard")){
            }

            else if (!strcmp(command, "st") || !strcmp(command,"show_trials")){
                showTrialsCmd(GSIP,GSport,PLID);
            }

            else if (!strcmp(command, "quit")){
                command_status = quitCmd(GSIP,GSport,PLID);
                if (command_status == ERROR)
                    fprintf(stderr, "Quit command error\n");
                else if (command_status == RESTART){
                    PLID = UNKNOWN;
                    trial_number = 1;
                    max_playtime = UNKNOWN;
                }
            }

            else if (!strcmp(command, "exit")){
                exit_application = exitCmd(GSIP,GSport,PLID);
                if(exit_application == ERROR){
                    fprintf(stderr, "Exit command error\n"); 
                }
            }
            else{
                fprintf(stderr, "Invalid command\n");
            }
        }
    }

    return 0;
}
