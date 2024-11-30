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

using namespace std;

#define MAXPORTSIZE 6        // Maximum port size
#define PORT "58001"         // Default port (58000 + GroupNo)
#define VERBOSEDEFAULT false // Default verbose mode

#define VERBOSEPREFIX "-v\0" // Verbose mode prefix
#define GSPORTPREFIX "-p\0"  // Gsport's prefix

#define FALSE 0
#define TRUE 1
#define ERROR 2

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

int main(int argc, char **argv)
{
    int udp_fd, tcp_fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    timeval timeout;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    fd_set read_fds;
    char client_request[128];

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

    // Configuração do endereço local (localhost)
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

    // Main server loop
    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(udp_fd, &read_fds);
        FD_SET(tcp_fd, &read_fds);
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int ready = select(FD_SETSIZE, &read_fds, (fd_set *)NULL, (fd_set *)NULL, &timeout);
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
        else if (FD_ISSET(tcp_fd, &read_fds))
        {
            // Data is available to read
            sockaddr_in recv_addr;
            socklen_t recv_len = sizeof(recv_addr);
            ssize_t received = recvfrom(tcp_fd, client_request, 128, 0,
                                        (struct sockaddr *)&recv_addr, &recv_len);

            if (received > 0)
            {
                client_request[received] = '\0';
            }
            else
                fprintf(stderr, "No data received\n");
        }
        else if (FD_ISSET(udp_fd, &read_fds)) {
            sockaddr_in recv_addr;
            socklen_t recv_len = sizeof(recv_addr);
            ssize_t received = recvfrom(udp_fd, client_request, 128, 0,
                                     (struct sockaddr*)&recv_addr, &recv_len);

            if (received > 0)
            {
                client_request[received] = '\0';
            }
            else
                fprintf(stderr, "No data received\n");
        }

        printf("[Client Request]: .%s.\n", client_request);
    }

    freeaddrinfo(res);
    close(udp_fd);
    close(tcp_fd);

    return 0;
}
