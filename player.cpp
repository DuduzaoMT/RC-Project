#include "common.h"
#include "player.h"

using namespace std;

// Verifies the arguments of the program once it is called
int verifyArg(char **user_args, int idx, const char *prefix, char *arg_to_change, const char *default_val)
{
    if (strcmp(user_args[idx], prefix) == 0)
    {
        if (strcmp(default_val, arg_to_change))
            return 1;

        strcpy(arg_to_change, user_args[idx + 1]);
    }
    return 0;
}

// Full UDP connection
int UDPInteraction(char *request, char *response, char *GSIP, char *GSport)
{

    int fd, errcode;
    fd_set read_fds;
    timeval timeout;
    ssize_t send, rec;
    socklen_t addrlen;
    struct addrinfo hints, *server_info, *client_info;

    // Send and receive messages
    int n_attempts = CONNECTIONATTEMPS;
    while (n_attempts > 0)
    {
        // UDP socket creation
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd == -1)
        {
            perror("socket");
            return 1;
        }

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;      // IPv4
        hints.ai_socktype = SOCK_DGRAM; // Socket UDP

        // Client info configuration
        errcode = getaddrinfo(NULL, GSport, &hints, &client_info);
        if (errcode != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
            close(fd);
            return 1;
        }

        // Server info configuration
        errcode = getaddrinfo(GSIP, GSport, &hints, &server_info);
        if (errcode != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
            return 1;
        }
        // Send message to server
        printf("[UDP request]: .%s.\n", request);
        send = sendto(fd, request, strlen(request), 0, server_info->ai_addr, server_info->ai_addrlen);
        if (send == -1)
        {
            perror("sendto");
            return 1;
        }

        // Receive message from server
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int ready = select(FD_SETSIZE, &read_fds, (fd_set *)NULL, (fd_set *)NULL, &timeout);
        if (ready < 0)
        {
            perror("select");
            close(fd);
            return 1;
        }
        else if (ready == 0)
        {
            fprintf(stderr, "No data received\n");
            close(fd);
        }
        else if (FD_ISSET(fd, &read_fds))
        {
            // Data is available to read
            sockaddr_in recv_addr;
            socklen_t recv_len = sizeof(recv_addr);
            ssize_t received = recvfrom(fd, response, 128, 0,
                                        (struct sockaddr *)&recv_addr, &recv_len);

            if (received > 0)
            {
                response[received] = '\0';
                break;
            }
            else
            {
                fprintf(stderr, "No data received\n");
            }
        }
        n_attempts--;
    }

    printf("[UDP response]: .%s.\n", response);

    freeaddrinfo(server_info);
    freeaddrinfo(client_info);
    close(fd);
    return 0;
}

// Full TCP connection
int TCPInteraction(char *request, char *response, char *GSIP, char *GSport)
{

    int fd, errcode, nbytes, nleft, nwritten, nread;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[GENERALSIZEBUFFER];

    // Create the socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1)
    {
        perror("Socket creation failed");
        return 1;
    }

    // Initialize the hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // Resolve server address
    errcode = getaddrinfo(GSIP, GSport, &hints, &res);
    if (errcode != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        return 1;
    }

    // Connect to the server
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    {
        perror("Connection failed");
        freeaddrinfo(res);
        close(fd);
        return 1;
    }

    nbytes = strlen(request);
    nleft = nbytes;

    while (nleft > 0)
    {
        printf("[TCP request]: .%s.\n", request);
        nwritten = write(fd, request, nleft);
        if (nwritten <= 0) /*error*/
            return 1;
        nleft -= nwritten;
        request += nwritten;
    }
    char file_name[128];
    int bytes;
    bool first = true;

    nleft = 2048;

    memset(response, 0, sizeof(response));
    char *last_digit_pointer = response;

    while (nleft > 0)
    {
        nread = read(fd, buffer, nleft);
        if (nread == -1) /*error*/
            return 1;
        else if (nread == 0)
            break; // closed by peer
        buffer[nread] = '\0';
        printf("[TCP response]: .%s.\n", buffer);
        nleft -= nread;
        strcpy(last_digit_pointer, buffer);
        last_digit_pointer += nread;
    }

    // Cleanup
    freeaddrinfo(res);
    close(fd);

    return 0;
}

int startCmd(char *arguments, char *GSIP, char *GSport, int *PLID, int *max_playtime, int *trial_number)
{

    int new_PLID, new_max_playtime;

    char PLID_buffer[USERINPUTBUFFER], max_playtime_buffer[USERINPUTBUFFER];
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    // Reset the buffers
    strcpy(PLID_buffer, "");
    strcpy(max_playtime_buffer, "");

    sscanf(arguments, "%s %s", PLID_buffer, max_playtime_buffer);
   
    // Verify the arguments
    if (verifyStartCmd(PLID_buffer, max_playtime_buffer) == ERROR)
    {
        fprintf(stderr, "Invalid arguments\n");
        return ERROR;
    }

    // Build request
    new_PLID = atoi(PLID_buffer);
    new_max_playtime = atoi(max_playtime_buffer);
    sprintf(request, "SNG %06d %03d\n", new_PLID, new_max_playtime);

    // Server response
    if (UDPInteraction(request, response, GSIP, GSport))
        return ERROR;

    if (!strncmp(response, "RSG OK\n", 7))
    {
        fprintf(stdout, "New Game started (max %d sec)\n", new_max_playtime);
    }
    else if (!strncmp(response, "RSG NOK\n", 8))
    {
        fprintf(stdout, "The player with PLID:%06d has an ongoing game\n", new_PLID);
        return ERROR;
    }
    else
        return ERROR;

    (*trial_number) = 1;
    (*PLID) = new_PLID;
    (*max_playtime) = new_max_playtime;

    return FALSE;
}

int tryCmd(char *arguments, char *GSIP, char *GSport, int *trial_number, int PLID)
{

    char C1, C2, C3, C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    int nB, nW, r_trial_number = -1;

    
    int n_args = sscanf(arguments, "%s %s %s %s", &C1, &C2, &C3, &C4);
    /*
    if (n_args != 4 || strlen(arguments) != 9)
    {
        fprintf(stderr, "Invalid Colors\n");
        return ERROR;
    }
    
    if(verifyTryCmd(C1, C2, C3, C4) == ERROR)
        return ERROR;
    */
    sprintf(request, "TRY %06d %c %c %c %c %d\n", PLID, C1, C2, C3, C4, *trial_number);

    // Exibição da resposta do servidor
    if (UDPInteraction(request, response, GSIP, GSport))
        return ERROR;

    if (!strncmp(response, "RTR OK", 6))
    {
        sscanf(response, "RTR OK %d %d %d\n", &r_trial_number, &nB, &nW);
        if (nB == 4)
        {
            fprintf(stdout, "Congrats, you guessed right! SK:%c %c %c %c\n", C1, C2, C3, C4);
            return RESTART;
        }
        fprintf(stdout, "nB = %d, nW=%d\n", nB, nW);
        (*trial_number)++;
    }
    else if (!strncmp(response, "RTR DUP", 7))
    {
        fprintf(stdout, "Secret Key guess repeats a previous trial`s guess: nB = %d, nW=%d\n", nB, nW);
    }
    // Invalid trial number
    else if (!strncmp(response, "RTR INV", 7))
    {
        fprintf(stderr, "Invalid trial number\n");
        return ERROR;
    }
    //(invalid PLID - not having an ongoing game f.e.)
    else if (!strncmp(response, "RTR NOK", 7))
    {
        fprintf(stderr, "Out of context\n");
        return ERROR;
    }
    // no more attemps available
    else if (!strncmp(response, "RTR ENT", 7))
    {
        sscanf(response, "RTR ENT %c %c %c %c\n", &C1, &C2, &C3, &C4);
        fprintf(stdout, "Secret Key was: %c %c %c %c\n", C1, C2, C3, C4);
        return RESTART;
    }
    // max time exceeded
    else if (!strncmp(response, "RTR ETM", 7))
    {
        sscanf(response, "RTR ETM %c %c %c %c\n", &C1, &C2, &C3, &C4);
        fprintf(stdout, "Secret Key was: %c %c %c %c\n", C1, C2, C3, C4);
        return RESTART;
    }
    else
        return ERROR;

    return false;
}

int quitCmd(char *GSIP, char *GSport, int PLID)
{

    char C1, C2, C3, C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    sprintf(request, "QUT %06d\n", PLID);

    if (UDPInteraction(request, response, GSIP, GSport))
        return ERROR;

    if (!strncmp(response, "RQT OK", 6))
    {
        sscanf(response, "RQT OK %c %c %c %c\n", &C1, &C2, &C3, &C4);
        fprintf(stdout, "Secret Key was: %c %c %c %c\n", C1, C2, C3, C4);
        return RESTART;
    }
    // PLID not have an ongoing game
    else if (!strncmp(response, "RQT NOK", 7))
    {
        fprintf(stdout, "There`s not an ongoing game\n");
    }
    else
        return ERROR;

    return false;
}

int exitCmd(char *GSIP, char *GSport, int PLID)
{
    char C1, C2, C3, C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    if (PLID != -1)
    {

        sprintf(request, "QUT %06d\n", PLID);

        if (UDPInteraction(request, response, GSIP, GSport))
            return ERROR;

        if (!strncmp(response, "RQT OK", 6))
        {
            sscanf(response, "RQT OK %c %c %c %c\n", &C1, &C2, &C3, &C4);
            fprintf(stdout, "Secret Key was: %c %c %c %c\n", C1, C2, C3, C4);
        }
        else if (!strncmp(response, "RQT ERR", 6))
            return ERROR;
    }
    return true;
}

int debugCmd(char *arguments, char *GSIP, char *GSport, int *PLID, int *max_playtime, int *trial_number)
{

    int new_PLID, new_max_playtime;

    char PLID_buffer[USERINPUTBUFFER], max_playtime_buffer[USERINPUTBUFFER], C1, C2, C3, C4;
    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER];

    // Reset the buffers
    strcpy(PLID_buffer, "");
    strcpy(max_playtime_buffer, "");

    sscanf(arguments, "%s %s %c %c %c %c", PLID_buffer, max_playtime_buffer, &C1, &C2, &C3, &C4);

    if (strlen(PLID_buffer) == 6 && isNumber(PLID_buffer))
        new_PLID = atoi(PLID_buffer);
    else
    {
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }

    if (isNumber(max_playtime_buffer))
    {
        new_max_playtime = atoi(max_playtime_buffer);
        if (new_max_playtime <= 0 || new_max_playtime > 600)
        {
            fprintf(stderr, "Invalid max_playtime\n");
            return ERROR;
        }
    }

    sprintf(request, "DBG %06d %03d %c %c %c %c\n", new_PLID, new_max_playtime, C1, C2, C3, C4);

    // Exibição da resposta do servidor
    if (UDPInteraction(request, response, GSIP, GSport))
        return ERROR;

    if (!strcmp(response, "RDB OK\n"))
    {
        fprintf(stdout, "New Game started (max %d sec)\n", new_max_playtime);
    }
    else if (!strcmp(response, "RDB NOK\n"))
    {
        fprintf(stdout, "The player with PLID:%06d has an ongoing game", new_PLID);
        return ERROR;
    }
    else
        return ERROR;

    (*trial_number) = 1;
    (*PLID) = new_PLID;
    (*max_playtime) = new_max_playtime;

    return false;
}

int showTrialsCmd(char *GSIP, char *GSport, int PLID)
{

    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER], f_name[USERINPUTBUFFER], *f_data,
        f_size_buffer[USERINPUTBUFFER], status[USERINPUTBUFFER];

    sprintf(request, "STR %06d\n", PLID);

    if (TCPInteraction(request, response, GSIP, GSport))
        return ERROR;

    if (!strncmp(response, "ERR", 3))
    {
        fprintf(stderr, "Error\n");
        return ERROR;
    }

    if (!strncmp(response, "RST NOK", 7))
    {
        fprintf(stderr, "No game found\n");
        return 0;
    }

    if (!strncmp(response, "RST ACT", 7) || !strncmp(response, "RST FIN", 7))
    {
        sscanf(response, "RST %s %s %s ", status, f_name, f_size_buffer);

        f_data = (char *)malloc(atoi(f_size_buffer) * sizeof(char) + 1);

        strcpy(f_data, response + 8 + strlen(f_name) + 1 + strlen(f_size_buffer) + 1);

        printf("%s", f_data);

        free(f_data);
    }

    if (!strcmp(status, "FIN"))
        return RESTART;

    return 0;
}

int scoreBoard(char *GSIP, char *GSport)
{

    char request[GENERALSIZEBUFFER], response[GENERALSIZEBUFFER], f_name[USERINPUTBUFFER], *f_data,
        f_size_buffer[USERINPUTBUFFER];

    int sb_fd;

    ssize_t bytes_written;

    sprintf(request, "SSB\n");

    if (TCPInteraction(request, response, GSIP, GSport))
        return ERROR;

    if (!strncmp(response, "ERR", 3))
    {
        fprintf(stderr, "Error\n");
        return ERROR;
    }

    if (!strncmp(response, "RSS EMPTY", 8))
    {
        fprintf(stderr, "No player has won yet\n");
        return 0;
    }

    if (!strncmp(response, "RSS OK", 6))
    {
        sscanf(response, "RSS OK %s %s ", f_name, f_size_buffer);

        f_data = (char *)malloc(atoi(f_size_buffer) * sizeof(char) + 1);

        strcpy(f_data, response + 7 + strlen(f_name) + 1 + strlen(f_size_buffer) + 2);

        printf("Scoreboard locally stored in: %s", f_name);

        sb_fd = open(f_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);

        if (sb_fd < 0)
        {
            free(f_data);
            fprintf(stderr, "Couldn't create file\n");
            return ERROR;
        }

        bytes_written = write(sb_fd, f_data, strlen(f_data));
        if (bytes_written < 0)
        {
            free(f_data);
            close(sb_fd);
            fprintf(stderr, "Couldn't write to file\n");
            return ERROR;
        }

        close(sb_fd);
        free(f_data);
    }

    return 0;
}

int main(int argc, char **argv)
{

    char GSIP[MAXIPSIZE] = LOCALHOST;
    char GSport[MAXPORTSIZE] = PORT;

    int trial_number = 1;
    int exit_application = false;
    int PLID = UNKNOWN;
    int max_playtime = UNKNOWN;

    int command_status = -1;

    // Argument verification
    for (int i = 1; i < argc; i += 2)
    {
        if (i + 1 >= argc)
        {
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }

        if (verifyArg(argv, i, GSIPPREFIX, GSIP, LOCALHOST))
        {
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }

        if (verifyArg(argv, i, GSPORTPREFIX, GSport, PORT))
        {
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }
    }

    printf("IP: %s\nPort: %s\n", GSIP, GSport);

    // Player controller

    while (exit_application != true)
    {
        // printf("-----\nPLID atual: %d\ntrial_number atual: %d\nmax_playtime atual: %d\n-----\n", PLID, trial_number, max_playtime);

        char command[USERINPUTBUFFER], arguments[USERINPUTBUFFER];
        scanf("%s", command);

        if (fgets(arguments, sizeof(arguments), stdin))
        {

            // commands with arguments
            if (!strcmp(command, "start"))
            {
                if (startCmd(arguments, GSIP, GSport, &PLID, &max_playtime, &trial_number) == ERROR)
                {
                    fprintf(stderr, "Start command error\n");
                }
            }
            else if (!strcmp(command, "try"))
            {
                command_status = tryCmd(arguments, GSIP, GSport, &trial_number, PLID);
                if (command_status == ERROR)
                    fprintf(stderr, "Try command error\n");
                else if (command_status == RESTART)
                {
                    PLID = UNKNOWN;
                    trial_number = 1;
                    max_playtime = UNKNOWN;
                }
            }
            else if (!strcmp(command, "debug"))
            {
                if (debugCmd(arguments, GSIP, GSport, &PLID, &max_playtime, &trial_number) == ERROR)
                {
                    fprintf(stderr, "Debug command error\n");
                }
            }
            // commands without arguments
            else if (!strcmp(command, "sb") || !strcmp(command, "scoreboard"))
            {
                command_status = scoreBoard(GSIP, GSport);
                if (command_status == ERROR)
                    fprintf(stderr, "Scoreboard command error\n");
            }

            else if (!strcmp(command, "st") || !strcmp(command, "show_trials"))
            {
                command_status = showTrialsCmd(GSIP, GSport, PLID);
                if (command_status == ERROR)
                    fprintf(stderr, "Show trials command error\n");
                else if (command_status == RESTART)
                {
                    PLID = UNKNOWN;
                    trial_number = 1;
                    max_playtime = UNKNOWN;
                }
            }

            else if (!strcmp(command, "quit"))
            {
                command_status = quitCmd(GSIP, GSport, PLID);
                if (command_status == ERROR)
                    fprintf(stderr, "Quit command error\n");
                else if (command_status == RESTART)
                {
                    PLID = UNKNOWN;
                    trial_number = 1;
                    max_playtime = UNKNOWN;
                }
            }

            else if (!strcmp(command, "exit"))
            {
                exit_application = exitCmd(GSIP, GSport, PLID);
                if (exit_application == ERROR)
                {
                    fprintf(stderr, "Exit command error\n");
                }
            }
            else
            {
                fprintf(stderr, "Invalid command\n");
            }
        }
    }

    return 0;
}
