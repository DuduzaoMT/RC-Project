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

#define GENERALSIZEBUFFER 2048 // General size to auxiliar buffers

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
    fd_set read_fds, test_fds;
    char client_request[GENERALSIZEBUFFER],server_response[GENERALSIZEBUFFER];


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

    FD_ZERO(&read_fds);
    FD_SET(udp_fd, &read_fds);
    FD_SET(tcp_fd, &read_fds);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    // Main server loop
    while (1)
    {
        memset(client_request,0,sizeof(client_request));
        memset(server_response,0,sizeof(server_response));
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
            // Data is available to read
            sockaddr_in recv_addr;
            socklen_t recv_len = sizeof(recv_addr);
            
            new_fd=accept(tcp_fd,(struct sockaddr*)&addr,&addrlen);
            if(new_fd==-1)
                return 1;

            int nleft = 2048;
            int nread = 0;
            int nbytes = 0;
            int nwritten = 0;
            char buffer[GENERALSIZEBUFFER];
            char *last_digit_pointer = client_request;

            while ((nread = read(new_fd, buffer, nleft)) != 0)
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

            sprintf(server_response,"aaaaaaa");
            nbytes = strlen(server_response);
            nleft = nbytes;
            last_digit_pointer = server_response;

            while (nleft > 0)
            {
                nwritten = write(new_fd, last_digit_pointer, nleft);
                if (nwritten <= 0) /*error*/
                    return 1;
                nleft -= nwritten;
                last_digit_pointer += nwritten;
                printf("[TCP package Write]: .%s.\n", last_digit_pointer);
            }

            if (strlen(client_request) <= 0)
                fprintf(stderr, "No data received\n");

            close(new_fd);
            
        }
        else if (FD_ISSET(udp_fd, &test_fds)) {
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
