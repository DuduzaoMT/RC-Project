#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

#define MAXPORTSIZE 6           // Tamanho máximo da porta
#define MAXIPSIZE 40            // Tamanho máximo do ip (IPV6 pode ir até 39)
#define PORT "58001"            // Porta default (58000 + GroupNo)
#define LOCALHOST "127.0.0.1"   // Host default (mesmo pc)

#define GSIPPREFIX "-n\0"       // Prefixo do argumento GSIP
#define GSPORTPREFIX "-p\0"     // Prefixo do argumento Gsport

int main(int argc, char **argv) {

    int fd, errcode, argValid;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    char buffer[128];

    char GSIP[MAXIPSIZE] = LOCALHOST;
    char GSport[MAXPORTSIZE] = PORT;

    for (int i = 1; i < argc; i+=2)
    {   
        if (i+1 >= argc){
            printf("Invalid arguments\n");
            exit(1);
        }

        if(strcmp(argv[i], GSIPPREFIX) == 0)
            strcpy(GSIP, argv[i+1]);
        
        if(strcmp(argv[i], GSPORTPREFIX) == 0)
            strcpy(GSport, argv[i+1]);
    }

    printf("IP: %s\nPort: %s\n", GSIP, GSport);
    

    // Criação do socket UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    // Configuração do endereço do servidor (localhost)
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
