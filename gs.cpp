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
            if (strcmp((char *)default_val, (char *)arg_to_change) || (idx) + 1 >= num_args)
                return ERROR;

            strcpy((char *)arg_to_change, user_args[idx + 1]);
            return TRUE;
        }
    }
    return FALSE;
}

// Verbose mode
int verboseMode(int verbose, char *PLID, char *request, char *ip, int port)
{
    if (!verbose)
        return 1;

    char clean_request[GENERALSIZEBUFFER];
    strncpy(clean_request, request, strlen(request) - 1);
    clean_request[strlen(request) - 1] = '\0';

    printf("[REQUEST]: %s, originated from: %s:%d, player id: %s\n", clean_request, ip, port, PLID);
    return 0;
}

/* - General helpers - */
void getColours(char *colours)
{

    char caracteres[] = {'R', 'G', 'B', 'Y', 'O', 'P'};
    int tamanho_conjunto = 6;

    for (int i = 0; i < 4; i++)
    {
        colours[i] = caracteres[rand() % tamanho_conjunto];
    }

    colours[4] = '\0';
}

int getBlackAndWhite(int *nB, int *nW, char *colours, char *guess_colours)
{

    for (int i = 0; i < 4; i++)
    {

        // right position
        if (colours[i] == guess_colours[i])
        {
            colours[i] = 'X';
            *nB += 1;
        }
        else
        {
            for (int j = 0; j < 4; j++)
            {
                if (guess_colours[i] == colours[j])
                {
                    colours[j] = 'X';
                    *nW += 1;
                    break;
                }
            }
        }
    }
    return 0;
}
/* ------------------- */

/* - Socket connections - */

// TCP connection behaviour
int TCPConnection(int tcp_fd, int verbose, sockaddr_in *addr)
{
    int nleft = 2048;
    int nread = 0;
    int nwritten = 0;
    char buffer[GENERALSIZEBUFFER], client_request[GENERALSIZEBUFFER], server_response[GENERALSIZEBUFFER];
    char *last_digit_pointer = client_request;
    char PLID[10], opcode[4];

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
        if (*(last_digit_pointer - 1) == '\n')
            break;
    }
    /* ---------------- */
    if (sscanf(client_request, "%*s %s", PLID) != 1)
    {
        strcpy(PLID, "Unknown");
    }

    verboseMode(verbose, PLID, client_request, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

    commandHandler(client_request, server_response);

    /* Writing packages */
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

// UDP connection behaviour
int UDPConnection(int udp_fd, sockaddr_in *addr, int verbose)
{
    char client_request[GENERALSIZEBUFFER], server_response[GENERALSIZEBUFFER];
    char PLID[7];
    int send;
    socklen_t addrlen = sizeof(*addr);

    memset(client_request, 0, sizeof(client_request));
    memset(server_response, 0, sizeof(server_response));

    /* Reading packages */
    ssize_t received = recvfrom(udp_fd, client_request, 128, 0,
                                (struct sockaddr *)addr, &addrlen);
    if (received > 0)
    {
        client_request[received] = '\0';
    }
    else
        fprintf(stderr, "No data received\n");

    printf("[UDP request]: .%s.\n", client_request);
    /* ---------------- */

    sscanf(client_request, "%*s %s", PLID);

    verboseMode(verbose, PLID, client_request, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

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
/* ---------------------- */

/* - File reading/writing helpers - */
// Checks if a game has already ended
int gameAlreadyEnded(char *file_name)
{
    char buffer[GENERALSIZEBUFFER], buffer1[USERINPUTBUFFER], buffer2[USERINPUTBUFFER], buffer3[USERINPUTBUFFER],
        buffer4[USERINPUTBUFFER], buffer5[USERINPUTBUFFER];
    time_t current_time, start_time;
    int total_time;
    FILE *player_fd;

    time(&current_time);

    player_fd = fopen(file_name, "r");
    if (!player_fd)
        return ERROR;

    fgets(buffer, sizeof(buffer), player_fd);
    sscanf(buffer, "%s %s %s %d %s %s %ld", buffer1, buffer2, buffer3, &total_time, buffer4, buffer5, &start_time);

    fclose(player_fd);

    if (total_time < (current_time - start_time))
        return true;
    return false;
}

// Stores the results of a game, moving the file to the appropriate directory
int storeResult(char *file_name, char code)
{

    char buffer[GENERALSIZEBUFFER], PLID[USERINPUTBUFFER], YYYYMMDD[USERINPUTBUFFER], HHMMSS[USERINPUTBUFFER];
    char new_file_name[GENERALSIZEBUFFER];
    int year, month, day, hour, minute, second;
    time_t current_time, start_time, finish_time;
    struct tm *struct_finish_time;
    int total_time, min_time;
    FILE *player_fd;

    memset(buffer, 0, sizeof(buffer));
    memset(PLID, 0, sizeof(PLID));
    memset(YYYYMMDD, 0, sizeof(YYYYMMDD));
    memset(HHMMSS, 0, sizeof(HHMMSS));
    memset(new_file_name, 0, sizeof(new_file_name));

    time(&current_time);

    player_fd = fopen(file_name, "a+");
    if (!player_fd)
        return ERROR;

    fgets(buffer, sizeof(buffer), player_fd);
    sscanf(buffer, "%s %*s %*s %d %s %s %ld", PLID, &total_time, YYYYMMDD, HHMMSS, &start_time);
    min_time = MIN(total_time, current_time - start_time);

    memset(buffer, 0, sizeof(buffer));
    finish_time = start_time + min_time;
    struct_finish_time = gmtime(&finish_time);
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", struct_finish_time->tm_year + 1900, struct_finish_time->tm_mon + 1, struct_finish_time->tm_mday,
            struct_finish_time->tm_hour, struct_finish_time->tm_min, struct_finish_time->tm_sec);
    fprintf(player_fd, "%s %d\n", buffer, min_time);

    fclose(player_fd);

    // Build new file name
    sscanf(YYYYMMDD, "%d-%d-%d", &year, &month, &day);
    sscanf(HHMMSS, "%d:%d:%d", &hour, &minute, &second);
    sprintf(new_file_name, "GAMES/%s/%04d%02d%02d_%02d%02d%02d_%c.txt", PLID, year, month, day, hour, minute, second, code);

    // Create new directory
    sprintf(buffer, "GAMES/%s", PLID);
    if (mkdir(buffer, 0777) != 0)
    {
        if (errno != EEXIST)
        {
            perror("mkdir");
            return ERROR;
        }
    }

    // Move the file
    if (rename(file_name, new_file_name) != 0)
    {
        perror("rename");
        return ERROR;
    }

    return 0;
}

// Checks if the guess is duplicate and if the trial number is correct
int getDupGuessAndTrialNumber(FILE *player_fd, char *guess_colours, bool *dup, int *trial_number)
{
    char line[USERINPUTBUFFER];
    char prev_guess[5];

    (*trial_number) = 1;
    (*dup) = false;
    while (fgets(line, sizeof(line), player_fd))
    {
        sscanf(line, "T: %s %*d %*d %*d", prev_guess);
        (*trial_number) += 1;
        if (!strcmp(prev_guess, guess_colours))
        {
            (*dup) = true;
        }
    }
    return 0;
}

// Stars a game
int startGame(char *PLID, char *time_buffer, char *colors, char mode, char *opcode)
{

    FILE *player_fd;
    int total_time;
    char f_name[GENERALSIZEBUFFER], first_line[GENERALSIZEBUFFER], buffer[GENERALSIZEBUFFER];
    time_t fulltime;
    struct tm *current_time;
    char time_str[20];

    sprintf(f_name, "GAMES/GAME_%s.txt", PLID);

    player_fd = fopen(f_name, "r");

    if (player_fd != NULL)
    {
        // File already exists
        fgets(first_line, sizeof(first_line), player_fd);

        if (fgets(buffer, sizeof(buffer), player_fd))
        {

            fclose(player_fd);

            // Game has not ended (inform player)
            if (!gameAlreadyEnded(f_name))
            {
                sprintf(opcode, "NOK");
                return 0;
            }

            // Game has ended (store the results)
            storeResult(f_name, 'T');
        }
        else
        {
            fclose(player_fd);
        }
    }

    // File didnt exist or game hasnt started
    player_fd = fopen(f_name, "w");

    // Build content
    total_time = atoi(time_buffer);
    time(&fulltime);
    current_time = gmtime(&fulltime);
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", current_time->tm_year + 1900, current_time->tm_mon + 1, current_time->tm_mday,
            current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    // Write to file
    fprintf(player_fd, "%s %c %s %d %s %ld\n", PLID, mode, colors, total_time, time_str, fulltime);
    fclose(player_fd);

    sprintf(opcode, "OK");
    return 0;
}

// Gets the trials made in a game
int readTrials(char *f_name, char *f_data, int active_game)
{
    char buffer[GENERALSIZEBUFFER], line[GENERALSIZEBUFFER];
    char *pointer_data;
    char C1, C2, C3, C4;
    int nb, nw;
    time_t current_time = 0, start_time = 0;
    int total_time, remaining_time;

    time(&current_time);

    memset(f_data, 0, sizeof(f_data));
    memset(buffer, 0, sizeof(buffer));

    pointer_data = f_data;

    FILE *fd = fopen(f_name, "r");

    // Reads first line
    fgets(buffer, sizeof(buffer), fd);
    sscanf(buffer, "%*s %*s %*s %d %*s %*s %ld", &total_time, &start_time);

    while (fgets(buffer, sizeof(buffer), fd))
    {

        if (strncmp(buffer, "T:", 2) == 0)
        {
            sscanf(buffer, "T: %c%c%c%c %d %d %*s", &C1, &C2, &C3, &C4, &nb, &nw);
            sprintf(line, "%c %c %c %c %d %d\n", C1, C2, C3, C4, nb, nw);
            strcpy(pointer_data, line);
            pointer_data += strlen(line);
        }
    }

    remaining_time = total_time - (current_time - start_time);
    if (remaining_time > 0)
    {
        sprintf(line, "%d", remaining_time);
        strcpy(pointer_data, line);
        fclose(fd);
    }
    else
    {
        sprintf(line, "Game already finished");
        strcpy(pointer_data, line);
        fclose(fd);
        // If the game was still active, store the results
        if (active_game)
            storeResult(f_name, 'T');
    }
    return 0;
}

// Gets the last game played by a player
int findLastGame(char *PLID, char *f_name)
{
    struct dirent **filelist;
    int nentries, found;
    char dirname[20];

    // Format directory name
    sprintf(dirname, "GAMES/%s/", PLID);

    // Read the directory and order it alphabetically
    nentries = scandir(dirname, &filelist, 0, alphasort);
    found = 0;

    // Check if the directory is empty
    if (nentries <= 0)
    {
        return 0;
    }
    else
    {

        while (nentries--)
        {
            // Ignore hidden files
            if (filelist[nentries]->d_name[0] != '.' && !found)
            {
                // Format the file name
                sprintf(f_name, "GAMES/%s/%s", PLID, filelist[nentries]->d_name);
                found = 1;
            }
            free(filelist[nentries]);
        }
        free(filelist);
    }

    return found;
}

int addScore(char *f_name)
{

    char buffer[GENERALSIZEBUFFER], time_str[USERINPUTBUFFER], new_file_name[GENERALSIZEBUFFER];
    char PLID[7], mode_full[6], colors[5], mode_char;
    int num_tries = 0, score = 0;
    time_t fulltime;
    struct tm *current_time;

    FILE *fd = fopen(f_name, "r");

    fgets(buffer, sizeof(buffer), fd);
    sscanf(buffer, "%s %c %s %*s %*s %*s %*s", PLID, &mode_char, colors);

    if (mode_char == 'P')
        strcpy(mode_full, "PLAY");
    else
        strcpy(mode_full, "DEBUG");

    // Count the number of tries needed to win
    while (fgets(buffer, sizeof(buffer), fd))
        if (strncmp(buffer, "T:", 2) == 0)
            num_tries+=1;

    fclose(fd);

    // Calculate the score
    score = ((MAXTRIES - (num_tries-1)) * 100) / MAXTRIES;

    // Get current time
    time(&fulltime);
    current_time = gmtime(&fulltime);
    sprintf(time_str, "%02d%02d%04d_%02d%02d%02d", current_time->tm_mday, current_time->tm_mon + 1, current_time->tm_year + 1900,
            current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    // Create the new score file
    sprintf(new_file_name, "SCORES/%03d_%s_%s.txt", score, PLID, time_str);
    fd = fopen(new_file_name, "w");
    
    fprintf(fd, "%03d %s %s %d %s\n", score, PLID, colors, num_tries, mode_full);
    fclose(fd);

    return 0;
}

// Reads the top 10 scores and returns the number of entries
int findTopScores(Scorelist *list) {
    struct dirent **filelist;
    int nentries, ifile;
    char fname[300];
    FILE *fp;
    char mode[8];

    // Get the list of files in the directory
    nentries = scandir("SCORES/", &filelist, 0, alphasort);
    if (nentries <= 0) {
        return 0; // No files found
    } else {
        ifile = 0;

        while (nentries--) {
            // Ignore hidden files and limit to 10 entries
            if (filelist[nentries]->d_name[0] != '.' && ifile < 10) {
                sprintf(fname, "SCORES/%s", filelist[nentries]->d_name);

                fp = fopen(fname, "r");
                if (fp != NULL) {
                    fscanf(fp, "%d %s %s %d %s",
                           &list->score[ifile],
                           list->PLID[ifile],
                           list->colors[ifile],
                           &list->num_tries[ifile],
                           mode);

                    strcpy(list->mode[ifile], mode);

                    fclose(fp);
                    ++ifile;    
                }
            }
            free(filelist[nentries]); 
        }
        free(filelist); 
    }

    list->nscores = ifile; 
    return ifile;          
}

/* -------------------------------- */

/* - Commands - */
int commandHandler(char *client_request, char *response)
{

    char opcode[4] = {0};
    strncpy(opcode, client_request, 3);

    if (!strcmp(opcode, "SNG"))
    {
        if (startCmd(client_request, response) == ERROR)
        {
            fprintf(stderr, "Error starting game\n");
            sprintf(response, "RSG ERR\n");
        }
    }
    else if (!strcmp(opcode, "TRY"))
    {
        if (tryCmd(client_request, response) == ERROR)
        {
            fprintf(stderr, "Error in try\n");
            sprintf(response, "RTR ERR\n");
        }
    }
    else if (!strcmp(opcode, "QUT"))
    {
        if (quitCmd(client_request, response) == ERROR)
        {
            fprintf(stderr, "Error in quit\n");
            sprintf(response, "RQT ERR\n");
        }
    }
    else if (!strcmp(opcode, "DBG"))
    {
        if (debugCmd(client_request, response) == ERROR)
        {
            fprintf(stderr, "Error in debug\n");
            sprintf(response, "RQT ERR\n");
        }
    }
    else if (!strcmp(opcode, "STR"))
    {
        if (showTrialsCmd(client_request, response) == ERROR)
        {
            fprintf(stderr, "Error in show trials\n");
            sprintf(response, "RST NOK\n");
        }
    }
    else if (!strcmp(opcode, "SSB"))
    {
        if (scoreboardCmd(client_request, response) == ERROR)
        {
            fprintf(stderr, "Error in scoreboard\n");
            sprintf(response, "RST NOK\n");
        }
    }
    else
    {
        fprintf(stderr, "Invalid command\n");
    }

    return 0;
}

int startCmd(char *client_request, char *response)
{
    char PLID_buffer[USERINPUTBUFFER], time_buffer[USERINPUTBUFFER], colors[5], opcode[4];

    memset(PLID_buffer, 0, sizeof(PLID_buffer));
    memset(time_buffer, 0, sizeof(time_buffer));

    if (sscanf(client_request, "SNG %s %s\n", PLID_buffer, time_buffer) != 2)
    {
        fprintf(stderr, "Invalid arguments\n");
        return ERROR;
    };

    // Verify the arguments
    if (verifyStartCmd(PLID_buffer, time_buffer) == ERROR)
    {
        fprintf(stderr, "Invalid arguments\n");
        return ERROR;
    }

    getColours(colors);

    startGame(PLID_buffer, time_buffer, colors, 'P', opcode);

    sprintf(response, "RSG %s\n", opcode);

    return 0;
}

int tryCmd(char *client_request, char *response)
{
    char C1, C2, C3, C4, C1_guess, C2_guess, C3_guess, C4_guess;
    char PLID_buffer[USERINPUTBUFFER], nT_buffer[USERINPUTBUFFER], f_name[GENERALSIZEBUFFER],
        colours[GENERALSIZEBUFFER], first_line[GENERALSIZEBUFFER];
    int trial_number = 1;
    bool duplicate_trial = false;
    memset(PLID_buffer, 0, sizeof(PLID_buffer));
    memset(nT_buffer, 0, sizeof(nT_buffer));
    FILE *player_fd;

    if (sscanf(client_request, "TRY %s %c %c %c %c %s\n", PLID_buffer, &C1_guess, &C2_guess, &C3_guess, &C4_guess, nT_buffer) != 6)
    {
        fprintf(stderr, "Invalid sintax\n");
        return ERROR;
    }

    if (strlen(PLID_buffer) != 6 || !isNumber(PLID_buffer))
    {
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }

    if (strlen(nT_buffer) != MAXTRIESLEN || !isNumber(nT_buffer))
    {
        fprintf(stderr, "Invalid trial number\n");
        return ERROR;
    }

    if (verifyTryCmd(C1_guess, C2_guess, C3_guess, C4_guess) == ERROR)
    {
        fprintf(stderr, "Invalid colors\n");
        return ERROR;
    }

    sprintf(f_name, "GAMES/GAME_%s.txt", PLID_buffer);

    // Check if the game has ended
    if (gameAlreadyEnded(f_name) == true)
    {
        // Game has ended (store the results)
        storeResult(f_name, 'T');
        sprintf(response, "RTR ETM %c %c %c %c\n", C1, C2, C3, C4);
        return 0;
    }

    // verify if the file exists
    player_fd = fopen(f_name, "r");
    if (player_fd == NULL)
    {
        sprintf(response, "RTR NOK\n");
        return 0;
    }

    char guess_colours[5];
    time_t current_time = 0, start_time = 0;
    int total_time, game_time;
    int nB = 0, nW = 0;

    time(&current_time);

    sprintf(guess_colours, "%c%c%c%c", C1_guess, C2_guess, C3_guess, C4_guess);

    // read first line
    fgets(first_line, sizeof(first_line), player_fd);
    sscanf(first_line, "%s %*s %s %d %*s %*s %ld", PLID_buffer, colours, &game_time, &start_time);
    sscanf(colours, "%c%c%c%c", &C1, &C2, &C3, &C4);

    getDupGuessAndTrialNumber(player_fd, guess_colours, &duplicate_trial, &trial_number);

    // verify if the guess is equal to some other guess

    fclose(player_fd);

    // compare the guess
    getBlackAndWhite(&nB, &nW, colours, guess_colours);

    // exception of the INV case
    if (duplicate_trial == true && trial_number-1 == atoi(nT_buffer)){
        sprintf(response, "RTR OK %d %d %d\n", trial_number-1, nB, nW);
        return 0;
    }

    if (duplicate_trial == true)
    {
        sprintf(response, "RTR DUP\n");
        return 0;
    }

    if (atoi(nT_buffer) != trial_number)
    {
        sprintf(response, "RTR INV\n");
        return 0;
    }

    if (trial_number > 8)
    {
        storeResult(f_name, 'F');
        sprintf(response, "RTR ENT %c %c %c %c\n", C1, C2, C3, C4);
        return 0;
    }

    total_time = current_time - start_time;

    // if exists we should open for "appending"
    player_fd = fopen(f_name, "a");

    // write to file
    fprintf(player_fd, "T: %c%c%c%c %d %d %d\n", C1_guess, C2_guess, C3_guess, C4_guess, nB, nW, total_time);

    fclose(player_fd);

    // Player won
    if (nB == 4)
    {
        // addScore must be done before storeResult
        addScore(f_name);
        storeResult(f_name, 'W');
    }

    sprintf(response, "RTR OK %d %d %d\n", trial_number, nB, nW);

    return 0;
}

int quitCmd(char *client_request, char *response)
{
    char PLID_buffer[USERINPUTBUFFER], f_name[GENERALSIZEBUFFER], colors[5], C1, C2, C3, C4;
    FILE *player_fd;

    memset(PLID_buffer, 0, sizeof(PLID_buffer));

    sscanf(client_request, "QUT %s\n", PLID_buffer);

    if (strlen(PLID_buffer) != 6 || !isNumber(PLID_buffer))
    {
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }

    sprintf(f_name, "GAMES/GAME_%s.txt", PLID_buffer);

    // verify if the file exists
    player_fd = fopen(f_name, "r");
    if (player_fd == NULL)
    {
        sprintf(response, "RQT NOK\n");
        return 0;
    }

    fscanf(player_fd, "%*s %*s %s %*d %*s %*s %*d", colors);
    sscanf(colors, "%c%c%c%c", &C1, &C2, &C3, &C4);
    fclose(player_fd);

    storeResult(f_name, 'Q');

    sprintf(response, "RQT OK %c %c %c %c\n", C1, C2, C3, C4);

    return 0;
}

int debugCmd(char *client_request, char *response)
{
    char PLID_buffer[USERINPUTBUFFER], time_buffer[USERINPUTBUFFER], C1, C2, C3, C4, opcode[4], colors[5];
    memset(PLID_buffer, 0, sizeof(PLID_buffer));
    memset(time_buffer, 0, sizeof(time_buffer));

    sscanf(client_request, "DBG %s %s %c %c %c %c\n", PLID_buffer, time_buffer, &C1, &C2, &C3, &C4);

    if (verifyStartCmd(PLID_buffer, time_buffer) == ERROR)
    {
        fprintf(stderr, "Invalid arguments\n");
        return ERROR;
    }

    if (verifyTryCmd(C1, C2, C3, C4) == ERROR)
    {
        fprintf(stderr, "Invalid colors\n");
        return ERROR;
    }

    sprintf(colors, "%c%c%c%c", C1, C2, C3, C4);

    startGame(PLID_buffer, time_buffer, colors, 'D', opcode);

    sprintf(response, "RDB %s\n", opcode);

    return 0;
}

int showTrialsCmd(char *client_request, char *response)
{

    char PLID_buffer[USERINPUTBUFFER], f_name[GENERALSIZEBUFFER], f_data[GENERALSIZEBUFFER];
    char opcode[4];
    bool found = false;
    long f_size;
    FILE *player_fd;

    if (sscanf(client_request, "STR %s\n", PLID_buffer) != 1 || strlen(client_request) != 11)
    {
        fprintf(stderr, "Invalid sintax\n");
        return ERROR;
    }

    if (strlen(PLID_buffer) != 6 || !isNumber(PLID_buffer))
    {
        fprintf(stderr, "Invalid PLID\n");
        return ERROR;
    }

    sprintf(f_name, "GAMES/GAME_%s.txt", PLID_buffer);

    player_fd = fopen(f_name, "r");

    if (player_fd == NULL)
    {
        // Player doesn't have an ongoing game
        sprintf(opcode, "FIN");

        // Find the last game
        found = findLastGame(PLID_buffer, f_name);

        if (!found)
        {
            return ERROR;
        }

        readTrials(f_name, f_data, false);
        f_size = strlen(f_data);
    }
    else
    {
        // Player has an ongoing game
        sprintf(opcode, "ACT");

        fclose(player_fd);

        readTrials(f_name, f_data, true);
        f_size = strlen(f_data);
    }

    sprintf(f_name, "STATE_%s.txt", PLID_buffer);

    sprintf(response, "RST %s %s %ld %s\n", opcode, f_name, f_size, f_data);

    return 0;
}

int scoreboardCmd(char *client_request, char *response)
{
    char line[GENERALSIZEBUFFER], f_data[GENERALSIZEBUFFER], *current_char, f_name[USERINPUTBUFFER];
    Scorelist list;
    int num_entries, f_size;
    static int scoreboard_id=0;

    current_char = f_data;

    num_entries = findTopScores(&list);

    if (num_entries == 0)
    {
        sprintf(response, "RSS EMPTY\n");
        return 0;
    }

    for (int i = 0; i < num_entries; i++)
    {
        sprintf(line, "%03d %s %s %d %s\n", list.score[i], list.PLID[i], list.colors[i], list.num_tries[i], list.mode[i]);
        strcpy(current_char, line);
        current_char += strlen(line);
    }
    
    f_size = strlen(f_data);

    scoreboard_id+=1;
    sprintf(f_name, "TOPSCORES_%d.txt", scoreboard_id);

    sprintf(response, "RSS OK %s %d %s\n", f_name, f_size, f_data);

    return 0;
}
/* ------------ */

int main(int argc, char **argv)
{
    int udp_fd, tcp_fd, errcode, opt = 1;
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

    // Remove "Address already in use" error
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        return 1;
    }

    // Localhost configuration
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = INADDR_ANY; // Sockets
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, GSport, &hints, &res);
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

    if (listen(tcp_fd, 5) < 0)
    {
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
        test_fds = read_fds;
        memset((void *)&timeout, 0, sizeof(timeout));
        timeout.tv_sec = 10;

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

            new_fd = accept(tcp_fd, (struct sockaddr *)&addr, &addrlen);
            if (new_fd == -1)
            {
                perror("accept");
                return 1;
            }

            pid_t pid = fork();
            if(pid == 0){
                close(tcp_fd);
                TCPConnection(new_fd, verbose, &addr);
                close(new_fd);
                exit(0);
            }else{
                close(new_fd);
            }
        }

        // Test for UDP connection
        else if (FD_ISSET(udp_fd, &test_fds))
        {
            UDPConnection(udp_fd, &addr, verbose);
        }
    }

    freeaddrinfo(res);
    close(udp_fd);
    close(tcp_fd);

    return 0;
}
