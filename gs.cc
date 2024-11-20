#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "58001" // Porta usada para comunicação

int main() {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    // Criação do socket UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    // Configuração do endereço local (localhost)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Socket UDP
    hints.ai_flags = AI_PASSIVE;    // Aceita conexões

    errcode = getaddrinfo(NULL, PORT, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    // Associa o socket ao endereço local
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("bind");
        exit(1);
    }

    // Loop para receber e responder mensagens
    while (1) {
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            perror("recvfrom");
            exit(1);
        }

        write(1, "Received: ", 10);
        write(1, buffer, n);

        // Envia de volta o mesmo conteúdo recebido
        n = sendto(fd, "RSG OK\n", 8, 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            perror("sendto");
            exit(1);
        }

    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}
