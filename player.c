#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "58001" // Porta usada para comunicação

int main(int argc, char **argv) {

    int fd, errcode, argValid;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    char buffer[128];

    int GSIP;
    char *GSport;

    // leitura do input
    if(argc == 5) {
        printf("verificação aqui\n");
        GSport = "58001";
        argValid = 1;
    }
     else {
        printf("No arguments\n");
        argValid = 0;
     } 

    const char* port = !argValid ? "58001" : GSport ;

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

    errcode = getaddrinfo("127.0.0.1", PORT, &hints, &res);
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
