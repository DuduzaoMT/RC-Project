#include <time.h>
#include "common.h"
#include "gs.h" 

using namespace std;

// Verifies user arguments
int verifyArg(char **user_args, int num_args, int idx, const char *prefix, void *arg_to_change, const void *default_val, bool single_argument)
{
    if (strcmp(user_args[idx], prefix) == 0)
    {
        if (single_argument)
        {
            if ((*(int *)default_val) != (*(int *)arg_to_change))
                return ERROR;

            (*(int *)arg_to_change) = true;
            return TRUE;
        }
        else
        {
            printf("%d, %d\n", (idx) + 1, num_args);
            if (strcmp((char *)default_val, (char *)arg_to_change) || (idx) + 1 >= num_args)
                return ERROR;

            strcpy((char *)arg_to_change, user_args[idx + 1]);
            return TRUE;
        }
    }
    return FALSE;
}

// Verbose mode
int verboseMode(int verbose, int PLID, char *request, char *ip, char *port)
{
    if (!verbose)
        return 1;

    char clean_request[GENERALSIZEBUFFER];
    strncpy(clean_request, request, strlen(request) - 1);

    printf("[REQUEST]: %s, originated from: %s:%s, player id: %d\n", clean_request, ip, port, PLID);
    return 0;
}

// TCP connection behaviour
int TCPConnection(int tcp_fd){
    int nleft = 2048;
    int nread = 0;
    int nwritten = 0;
    char buffer[GENERALSIZEBUFFER], client_request[GENERALSIZEBUFFER], server_response[GENERALSIZEBUFFER];
    char *last_digit_pointer = client_request;

    memset(client_request, 0, sizeof(client_request));
    memset(server_response, 0, sizeof(server_response));

    /* Reading packages */
    while ((nread = read(tcp_fd, buffer, nleft)) != 0)
    {   
        if (nread == -1) /*error*/
            return 1;
        buffer[nread] = '\0';
        printf("[TCP package Read]: .%s.\n", buffer);
        nleft -= nread;
        strcpy(last_digit_pointer, buffer);
        last_digit_pointer += nread;
        if (*(last_digit_pointer-1) == '\n')
            break;
    }
    /* ---------------- */

    /* Writing packages */
    sprintf(server_response,"aaaaaaa");
    nleft = strlen(server_response);
    last_digit_pointer = server_response;
    printf("[TCP package Write]: .%s.\n", server_response);
    while (nleft > 0)
    {
        nwritten = write(tcp_fd, last_digit_pointer, nleft);
        if (nwritten <= 0) /*error*/
            return 1;
        nleft -= nwritten;
        last_digit_pointer += nwritten;
    }
    /* ---------------- */

    close(tcp_fd);
    return 0;
}

// UOP connection behaviour
int UDPConnection(int udp_fd, sockaddr_in *addr){
    char client_request[GENERALSIZEBUFFER], server_response[GENERALSIZEBUFFER];
    int send;
    socklen_t addrlen = sizeof(*addr);

    memset(client_request, 0, sizeof(client_request));
    memset(server_response, 0, sizeof(server_response));

    /* Reading packages */
    ssize_t received = recvfrom(udp_fd, client_request, 128, 0,
                                (struct sockaddr*)addr, &addrlen);
    if (received > 0)
    {
        client_request[received] = '\0';
    }
    else
        fprintf(stderr, "No data received\n");

    printf("[UDP request]: .%s.\n", client_request);
    /* ---------------- */

    commandHandler(client_request, server_response);

    /* Writing packages */
    send = sendto(udp_fd, server_response, strlen(server_response), 0, (struct sockaddr *)addr, addrlen);
    if (send == -1)
    {
        perror("sendto");
        return 1;
    }
    printf("[UDP response]: .%s.\n", server_response);
    /* ---------------- */

    return 0;
}

int commandHandler(char *client_request, char *response){

    char opcode[4];
    strncpy(opcode, client_request, 3);

    if (!strcmp(opcode, "SNG")){
        if (startCmd(client_request, response) == ERROR){
            fprintf(stderr, "Error starting game\n");
            sprintf(response, "RSG ERR\n");
            return 1;
        }
    }
    else if (!strcmp(opcode, "TRY")){
        if (tryCmd(client_request, response) == ERROR){
            fprintf(stderr, "Error in try\n");
            sprintf(response, "RTR ERR\n");
            return 1;
        }
    }
    else if (!strcmp(opcode, "QUT")){
       if (quitCmd(client_request, response) == ERROR){
            fprintf(stderr, "Error in quit\n");
            sprintf(response, "RQT ERR\n");
            return 1;
        }
    }
    else if (!strcmp(opcode, "DBG")){
        if (debugCmd(client_request, response) == ERROR){
            fprintf(stderr, "Error in debug\n");
            sprintf(response, "RQT ERR\n");
            return 1;
        }
    }
    else if (!strcmp(opcode, "STR")){
        /* code */
    }
    else if (!strcmp(opcode, "SSB")){
        /* code */
    }
    else {
        fprintf(stderr, "Invalid command\n");
        return 1;
    }

    return 0;
}

void getColours(char *colours){

    char caracteres[] = {'R', 'G', 'B', 'Y', 'O', 'P'};
    int tamanho_conjunto = 6;

    for (int i = 0; i < 4; i++) {
        colours[i] = caracteres[rand() % tamanho_conjunto];
    }

    colours[4] = '\0';
}

int startCmd(char *client_request, char *response){

    FILE *player_fd;
    int time_int;
    char PLID_buffer[USERINPUTBUFFER], time_buffer[USERINPUTBUFFER],f_name[GENERALSIZEBUFFER], first_line[GENERALSIZEBUFFER],
         buffer[GENERALSIZEBUFFER];
    time_t fulltime;
    struct tm *current_time;
    char time_str[20],colours[5];

    memset(PLID_buffer, 0, sizeof(PLID_buffer));
    memset(time_buffer, 0, sizeof(time_buffer));
    
    if (sscanf(client_request, "SNG %s %s\n", PLID_buffer, time_buffer) != 2){
        fprintf(stderr, "Invalid arguments\n");
        return ERR;
    };

    // Verify the arguments
    if (verifyStartCmd(PLID_buffer, time_buffer) == ERROR)
    {
        fprintf(stderr, "Invalid arguments\n");
        return ERR;
    }

    sprintf(f_name,"GAMES/GAME_%s.txt",PLID_buffer);

    player_fd = fopen(f_name, "w+");

    if(!player_fd){
        return ERR;
    }

    if (fgets(first_line, sizeof(first_line), player_fd) == NULL)
    {
        
    }
    else
    {
        if (fgets(buffer, sizeof(buffer), player_fd) != NULL)
        {
            return NOK;
        }
        
    }
    
    

    /*
    if(player_fd == NULL){
        player_fd = fopen(f_name, "w");
        if (player_fd == NULL) {
            perror("Erro ao criar o ficheiro");
            return 1;
        }

        // content
        time_int = atoi(time_buffer);
        time(&fulltime);
        current_time = gmtime(&fulltime);
        sprintf(time_str,"%04d-%02d-%02d %02d:%02d:%02d",current_time->tm_year+1900,current_time->tm_mon+1,current_time->tm_mday,
        current_time->tm_hour,current_time->tm_min,current_time->tm_sec);
        getColours(colours);
        sprintf(f_data,"%s P %s %d %s %ld",PLID_buffer,colours,time_int,time_str,fulltime);

        // write to game file
        if (write(player_fd, f_data, strlen(f_data)) < 0)
        {
            close(player_fd);
            fprintf(stderr, "Couldn't write to file\n");
            return ERROR;
        }
    }
    // FILE exists
    else{
        
        if (read(player_fd, f_data,) < 0)
        {
            close(player_fd);
            fprintf(stderr, "Couldn't write to file\n");
            return ERROR;
        }
    }*/

    // Build response
    printf("PLID: %s, time:%s\n", PLID_buffer, time_buffer);
    sprintf(response, "RSG OK\n");

    return 0;
}

int tryCmd(char *client_request, char *response){
    char C1, C2, C3, C4;
    char PLID_buffer[USERINPUTBUFFER], nT_buffer[USERINPUTBUFFER];
    memset(PLID_buffer, 0, sizeof(PLID_buffer));
    memset(nT_buffer, 0, sizeof(nT_buffer));

    if(sscanf(client_request, "TRY %s %c %c %c %c %s\n", PLID_buffer, &C1, &C2, &C3, &C4, nT_buffer)!= 6){
        fprintf(stderr, "Invalid sintax\n");
        return ERROR;
    }

    if (strlen(PLID_buffer) != 6 || !isNumber(PLID_buffer)){
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }

    if (strlen(nT_buffer) != MAXTRIESLEN || !isNumber(nT_buffer)){
        fprintf(stderr, "Invalid trial number\n");
        return ERROR;
    }
        
    if (verifyTryCmd(C1, C2, C3, C4) == ERROR){
        fprintf(stderr, "Invalid colors\n");
        return ERROR;
    }
    
    // Build response
    printf("PLID: %s; Colors: %c, %c, %c, %c; Trial: %s\n", PLID_buffer, C1, C2, C3, C4, nT_buffer);
    sprintf(response, "RTR NOK\n");

    return 0;
}

int quitCmd(char *client_request, char *response){
    char PLID_buffer[USERINPUTBUFFER];
    memset(PLID_buffer, 0, sizeof(PLID_buffer));

    sscanf(client_request, "QUT %s\n", PLID_buffer);

    if (strlen(PLID_buffer) != 6 || !isNumber(PLID_buffer)){
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }

    // Build response
    printf("PLID: %s\n", PLID_buffer);
    sprintf(response, "RQT NOK\n");

    return 0;
}

int debugCmd(char *client_request, char *response){
    char PLID_buffer[USERINPUTBUFFER], time_buffer[USERINPUTBUFFER], C1, C2, C3, C4;
    memset(PLID_buffer, 0, sizeof(PLID_buffer));
    memset(time_buffer, 0, sizeof(time_buffer));

    sscanf(client_request, "DBG %s %s %c %c %c %c\n", PLID_buffer, time_buffer, &C1, &C2, &C3, &C4);

    if (verifyStartCmd(PLID_buffer, time_buffer) == ERROR){
        fprintf(stderr, "Invalid arguments\n");
        return ERROR;
    }

    if (verifyTryCmd(C1, C2, C3, C4) == ERROR){
        fprintf(stderr, "Invalid colors\n");
        return ERROR;
    }

    // Build response
    printf("PLID: %s; Time: %s; Colors: %c, %c, %c, %c\n", PLID_buffer, time_buffer, C1, C2, C3, C4);
    sprintf(response, "RDB OK\n");

    return 0;
}

int main(int argc, char **argv)
{
    int udp_fd, tcp_fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    timeval timeout;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    fd_set read_fds, test_fds;

    char GSport[MAXPORTSIZE] = PORT;
    int verbose = VERBOSEDEFAULT, verbose_default = VERBOSEDEFAULT;

    // Argument verification
    int i = 1, verification;
    while (i < argc)
    {

        verification = verifyArg(argv, argc, i, VERBOSEPREFIX, &verbose, &verbose_default, true);

        if (verification == TRUE)
        {
            i++;
            continue;
        }
        else if (verification == ERROR)
        {
            fprintf(stderr, "Invalid arguments\n");
            return 1;
        }

        verification = verifyArg(argv, argc, i, GSPORTPREFIX, GSport, PORT, false);

        if (verification == TRUE)
        {
            i += 2;
            continue;
        }
        else if (verification == ERROR)
        {
            fprintf(stderr, "Invalid arguments\n");
            return 1;
        }

        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    printf("Port: %s\nVerbose: %d\n", GSport, verbose);

    // UDP socket creation
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd == -1)
    {
        perror("socket");
        return 1;
    }

    // TCP socket creation
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd == -1)
    {
        perror("socket");
        return 1;
    }

    // Localhost configuration
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = INADDR_ANY; // Sockets
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, PORT, &hints, &res);
    if (errcode != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        return 1;
    }

    n = bind(udp_fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    {
        perror("bind");
        return 1;
    }

    n = bind(tcp_fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    {
        perror("bind");
        return 1;
    }

    if (listen(tcp_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&read_fds);
    FD_SET(udp_fd, &read_fds);
    FD_SET(tcp_fd, &read_fds);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    // Main server loop
    while (1)
    {
        test_fds=read_fds; 
        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=10;

        int ready = select(FD_SETSIZE, &test_fds, (fd_set *)NULL, (fd_set *)NULL, &timeout);
        if (ready < 0)
        {
            perror("select");
            close(udp_fd);
            close(tcp_fd);
            return 1;
        }
        else if (ready == 0)
        {
            fprintf(stderr, "No data received\n");
        }

        // Test for TCP connection
        else if (FD_ISSET(tcp_fd, &test_fds))
        {
            int new_fd;

            new_fd=accept(tcp_fd,(struct sockaddr*)&addr,&addrlen);
            if(new_fd==-1){
                perror("accept");
                return 1;
            }
            
            TCPConnection(new_fd);
        }

        // Test for UDP connection
        else if (FD_ISSET(udp_fd, &test_fds)) {
            UDPConnection(udp_fd, &addr);
        }
    }

    freeaddrinfo(res);
    close(udp_fd);
    close(tcp_fd);

    return 0;
}
