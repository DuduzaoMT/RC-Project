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
    hints.ai_socktype = SOCK_DGRAM;  // Socket UDP
    hints.ai_flags = AI_PASSIVE;     // Aceita conexões

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
        
        char cmd[3]; 
        int PLID;

        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            perror("recvfrom");
            exit(1);
        }
    
        write(1, "Received: ", 10);
        write(1, buffer, n);

        // Envia de volta o mesmo conteúdo recebido
        if (!strncmp(buffer,"SNG",3)){
            n = sendto(fd, "RSG OK\n", 8, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1) {
                perror("sendto");
                exit(1);
            }
        }
        else if(!strncmp(buffer,"TRY",3)){
            char C1,C2,C3,C4;
            int trial_number,nW = 1,nB = 2;
            char response[128];

            sscanf(buffer,"TRY %06d %c %c %c %c %d\n", &PLID, &C1,&C2,&C3,&C4,&trial_number);
            sprintf(response,"RTR OK %d %d %d\n",trial_number,nB,nW);
            //sprintf(response,"RTR ENT %c %c %c %c\n",C1,C2,C3,C4);
            //sprintf(response,"RTR ENT %c %c %c %c\n",C1,C2,C3,C4);
            n = sendto(fd, response, 15, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1) {
                perror("sendto");
                exit(1);
            }
        }
        else if(!strncmp(buffer,"QUIT",3)){
            n = sendto(fd, "RQT OK", 6, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1) {
                perror("sendto");
                exit(1);
            }
        }
    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}
